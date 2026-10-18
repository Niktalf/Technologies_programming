#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cli.h"
#include "demos.h"
#include "record.h"
#include "store.h"

/* Демонстрации прошлых практик. Они нарочно работают с внутренними
   модулями библиотеки — таблицей и форматом записи — напрямую, потому
   что показывают именно внутреннее устройство. Обычному клиенту
   библиотеки, вроде справочника из demo/, сюда заглядывать незачем.
   Каждая возвращает CLI_OK или CLI_ERROR, как команда оболочки. */

static void print_stats(const Store *store) {
    StoreStats stats;
    store_stats(store, &stats);
    printf("Keys: %d of %d (%d%%)\n",
           stats.count, stats.capacity, stats.count * 100 / stats.capacity);
    printf("Cells marked as deleted: %d\n", stats.deleted);
    printf("Keys not in their home cell: %d\n", stats.displaced);
    printf("Longest probe chain: %d\n", stats.longest_probe);
    printf("Table rebuilds: %ld\n", stats.rehash_count);
}

static double seconds_since(clock_t start) {
    return (double)(clock() - start) / CLOCKS_PER_SEC;
}

/* Ищет ключи key0 … key(n-1) указанной функцией поиска и возвращает,
   сколько найдено. Время поиска возвращается через elapsed. */
static long search_all(const Store *store, long n,
                       StoreStatus (*get)(const Store *, const char *, const char **),
                       double *elapsed) {
    long found = 0;
    clock_t start = clock();
    for (long i = 0; i < n; ++i) {
        char key[32];
        snprintf(key, sizeof key, "key%ld", i);
        const char *value = NULL;
        if (get(store, key, &value) == STORE_OK) {
            ++found;
        }
    }
    *elapsed = seconds_since(start);
    return found;
}

/* Кладёт n ключей и ищет каждый двумя способами. Хранилище для замеров
   отдельное, чтобы не засорять пользовательское. Сама структура маленькая —
   таблица в куче, — поэтому она локальная; таблица растёт сама. */
int demo_bench(long n) {
    if (n <= 0) {
        printf("The number of keys must be positive\n");
        return CLI_ERROR;
    }
    Store bench;
    if (!store_init(&bench)) {
        printf("Not enough memory\n");
        return CLI_ERROR;
    }
    for (long i = 0; i < n; ++i) {
        char key[32];
        snprintf(key, sizeof key, "key%ld", i);
        store_put(&bench, key, "v", NULL);
    }

    double t_hash = 0.0;
    long found_hash = search_all(&bench, n, store_get, &t_hash);
    double t_linear = 0.0;
    long found_linear = search_all(&bench, n, store_get_linear, &t_linear);

    printf("Keys: %ld\n", n);
    printf("Hash:    found %ld in %.4f s\n", found_hash, t_hash);
    printf("Linear:  found %ld in %.4f s\n", found_linear, t_linear);
    if (t_hash > 0.0) {
        printf("Hash is %.0f times faster\n", t_linear / t_hash);
    }
    print_stats(&bench);
    store_destroy(&bench);
    return CLI_OK;
}

/* Подбирает два ключа с одной домашней ячейкой, кладёт оба, удаляет первый
   и ищет второй. Если удалённую ячейку пометить пустой, второй не найдётся. */
int demo_collision(void) {
    /* Ключи подбираются уже в том хранилище, где будут лежать:
       номер домашней ячейки зависит от размера таблицы. */
    Store scratch;
    if (!store_init(&scratch)) {
        printf("Not enough memory\n");
        return CLI_ERROR;
    }
    const char *first = "a0";
    int home = store_home(&scratch, first);

    /* Перебираем a1, a2, … пока не найдём ключ с той же домашней ячейкой. */
    char second[32] = "";
    int found = 0;
    for (long i = 1; i < 1000000 && !found; ++i) {
        snprintf(second, sizeof second, "a%ld", i);
        found = store_home(&scratch, second) == home;
    }
    if (!found) {
        printf("Could not find a collision.\n");
        store_destroy(&scratch);
        return CLI_OK;
    }

    store_put(&scratch, first, "first", NULL);
    store_put(&scratch, second, "second", NULL);
    printf("Keys %s and %s fall into the same cell %d.\n", first, second, home);

    store_remove(&scratch, first);
    printf("Deleted %s.\n", first);

    const char *value = NULL;
    if (store_get(&scratch, second, &value) == STORE_OK) {
        printf("Key %s found: %s\n", second, value);
    } else {
        printf("Key %s is lost: the deleted cell broke the probe chain.\n", second);
    }
    store_destroy(&scratch);
    return CLI_OK;
}

/* Печатает байты буфера шестнадцатеричными числами. */
static void print_bytes(const uint8_t *buffer, size_t size) {
    printf("Bytes:");
    for (size_t i = 0; i < size; ++i) {
        printf(" %02X", (unsigned)buffer[i]);
    }
    printf("\n");
}

/* Практика 2: сборка записи журнала, разбор и обнаружение порчи.
   Такими записями библиотека пишет журнал на диск. */
int demo_record(const char *key, const char *value) {
    uint8_t buffer[RECORD_MAX_SIZE];
    size_t size = 0;
    RecordStatus status = record_build(buffer, sizeof buffer,
                                       record_flags_set(0, RECORD_FLAG_PUT), key, value, &size);
    if (status != RECORD_OK) {
        printf("Build error: %s\n", record_status_text(status));
        return CLI_ERROR;
    }
    printf("Record size: %zu bytes (header %d + key %zu + value %zu)\n",
           size, RECORD_HEADER_SIZE, strlen(key), strlen(value));
    print_bytes(buffer, size);

    RecordHeader header;
    status = record_parse(buffer, size, &header);
    printf("Parse: %s\n", record_status_text(status));
    record_flags_print(header.flags);
    printf("Key length: %u, value length: %u, checksum: 0x%02X\n",
           (unsigned)header.key_length, (unsigned)header.value_length,
           (unsigned)header.checksum);

    /* Порча байта данных: длина сходится, порчу ловит контрольная сумма.
       Повторное исключающее ИЛИ с той же маской возвращает байт обратно. */
    buffer[size - 1] ^= 0x01u;
    status = record_parse(buffer, size, &header);
    printf("After corrupting the last byte: %s\n", record_status_text(status));
    buffer[size - 1] ^= 0x01u;

    /* Порча длины ключа: её ловит проверка размера, которая стоит раньше,
       потому что по длине мы будем читать файл. */
    buffer[2] ^= 0x01u;
    status = record_parse(buffer, size, &header);
    printf("After corrupting the key length: %s\n", record_status_text(status));
    buffer[2] ^= 0x01u;
    return CLI_OK;
}

