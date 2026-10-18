#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kvstore.h"
#include "record.h"
#include "store.h"

/* Устройство базы снаружи не видно: в публичном заголовке только имя типа.
   Всё, что ниже объявлено static, — внутренние функции с внутренним
   связыванием: из других файлов их не вызвать и не увидеть. */
struct KvStore {
    Store table;   /* индекс в памяти: ключ -> значение */
    FILE *journal; /* журнал на диске, открыт на чтение и запись */
    char *path;    /* путь к журналу, в куче */
    long records;  /* записей в журнале */
    long garbage;  /* из них уже не нужных */
    long broken;   /* записей, отброшенных при открытии как повреждённые */
};

const char *kv_status_text(KvStatus status) {
    switch (status) {
    case KV_OK:
        return "success";
    case KV_ERR_ARG:
        return "invalid argument";
    case KV_ERR_MEMORY:
        return "not enough memory";
    case KV_ERR_NOT_FOUND:
        return "key not found";
    case KV_ERR_OPEN:
        return "cannot open file";
    case KV_ERR_IO:
        return "journal read or write error";
    default:
        return "unknown error";
    }
}

/* Дописывает запись в конец журнала и сбрасывает её на диск.
   Сброс обязателен: без него при аварии потеряется не последняя
   операция, а всё, что осело в буфере. */
static KvStatus append(KvStore *store, unsigned char flags, const char *key, const char *value) {
    unsigned char buffer[RECORD_MAX_SIZE];
    size_t size = 0;
    if (record_build(buffer, sizeof buffer, flags, key, value, &size) != RECORD_OK) {
        return KV_ERR_ARG;
    }
    if (fwrite(buffer, 1, size, store->journal) != size || fflush(store->journal) != 0) {
        return KV_ERR_IO;
    }
    ++store->records;
    return KV_OK;
}

/* Чем кончилась попытка прочитать запись из журнала. */
typedef enum {
    READ_RECORD, /* целая запись с верной контрольной суммой */
    READ_END,    /* файл кончился ровно на границе записи */
    READ_BROKEN  /* оборванная или испорченная запись: дальше пригодных данных нет */
} ReadResult;

/* Читает одну запись в buffer. Длины из заголовка проверяются до чтения
   по ним: испорченная длина не заставит читать за пределами буфера. */
static ReadResult read_record(FILE *file, unsigned char *buffer, RecordHeader *parsed) {
    size_t got = fread(buffer, 1, RECORD_HEADER_SIZE, file);
    if (got == 0) {
        return READ_END;
    }
    if (got != RECORD_HEADER_SIZE) {
        return READ_BROKEN; /* оборван уже заголовок */
    }
    record_unpack_header(buffer, parsed);
    if (parsed->key_length == 0 || parsed->key_length > RECORD_KEY_MAX
        || parsed->value_length > RECORD_VALUE_MAX) {
        return READ_BROKEN;
    }
    size_t body = parsed->key_length + parsed->value_length;
    if (fread(buffer + RECORD_HEADER_SIZE, 1, body, file) != body) {
        return READ_BROKEN; /* запись оборвана: программу сняли на середине */
    }
    if (record_parse(buffer, RECORD_HEADER_SIZE + body, parsed) != RECORD_OK) {
        return READ_BROKEN; /* не сошлась контрольная сумма */
    }
    return READ_RECORD;
}

/* Применяет прочитанную запись к индексу. Побеждает последняя запись
   о ключе, прежние становятся мусором. */
static KvStatus apply_record(KvStore *store, const unsigned char *buffer,
                             const RecordHeader *parsed) {
    char key[RECORD_KEY_MAX + 1];
    char value[RECORD_VALUE_MAX + 1];
    memcpy(key, buffer + RECORD_HEADER_SIZE, parsed->key_length);
    key[parsed->key_length] = '\0';
    memcpy(value, buffer + RECORD_HEADER_SIZE + parsed->key_length, parsed->value_length);
    value[parsed->value_length] = '\0';

    if (record_flags_has(parsed->flags, RECORD_FLAG_TOMBSTONE)) {
        StoreStatus status = store_remove(&store->table, key);
        if (status == STORE_OK) {
            ++store->garbage; /* удалённая пара стала мусором */
        } else if (status != STORE_ERR_NOT_FOUND) {
            return KV_ERR_MEMORY;
        }
        ++store->garbage; /* и сама запись об удалении тоже */
        return KV_OK;
    }
    int was_present = 0;
    if (store_put(&store->table, key, value, &was_present) != STORE_OK) {
        return KV_ERR_MEMORY;
    }
    if (was_present) {
        ++store->garbage;
    }
    return KV_OK;
}

/* Читает журнал от начала и собирает индекс. Первая запись, которая
   не читается целиком или не сходится по контрольной сумме, означает
   конец пригодных данных: всё дальше отбрасывается. Через out_good_end
   сообщает, где кончаются целые записи. */
static KvStatus recover(KvStore *store, long *out_good_end) {
    unsigned char buffer[RECORD_MAX_SIZE];
    long good_end = 0;
    KvStatus status = KV_OK;
    ReadResult result = READ_RECORD;
    while (status == KV_OK && result == READ_RECORD) {
        RecordHeader parsed;
        result = read_record(store->journal, buffer, &parsed);
        if (result == READ_RECORD) {
            status = apply_record(store, buffer, &parsed);
            ++store->records;
            good_end = ftell(store->journal);
            if (good_end < 0) {
                status = KV_ERR_IO;
            }
        } else if (result == READ_BROKEN) {
            ++store->broken;
        }
    }
    /* Ошибка чтения выглядит для fread как конец файла: различаем их
       через ferror, иначе сбой диска сошёл бы за короткий журнал. */
    if (status == KV_OK && ferror(store->journal)) {
        status = KV_ERR_IO;
    }
    *out_good_end = good_end;
    return status;
}

/* Копирует первые length байт файла from в файл to. Возвращает 1 при успехе. */
static int copy_prefix(FILE *from, FILE *to, long length) {
    unsigned char chunk[4096];
    long left = length;
    int ok = fseek(from, 0, SEEK_SET) == 0;
    while (ok && left > 0) {
        size_t want = left < (long)sizeof chunk ? (size_t)left : sizeof chunk;
        ok = fread(chunk, 1, want, from) == want && fwrite(chunk, 1, want, to) == want;
        left -= (long)want;
    }
    return ok;
}

/* Отрезает от журнала повреждённый хвост: копирует целые записи
   во временный файл и ставит его на место старого.

   Просто писать дальше поверх хвоста нельзя. Новая запись короче
   старого мусора, и остаток мусора будет снова прочитан при следующем
   открытии — а при неудачном совпадении байтов даже принят за запись.
   В стандартной библиотеке C нет функции «обрезать файл», поэтому
   хвост отрезается переписыванием. Случается это только после аварии. */
static KvStatus cut_tail(KvStore *store, long good_end) {
    char temp_path[1024];
    if (snprintf(temp_path, sizeof temp_path, "%s.tmp", store->path) >= (int)sizeof temp_path) {
        return KV_ERR_ARG;
    }
    FILE *temp = fopen(temp_path, "wb");
    if (temp == NULL) {
        return KV_ERR_OPEN;
    }
    int copied = copy_prefix(store->journal, temp, good_end);
    /* Результат fclose проверяется: данные из буфера уходят на диск именно тогда. */
    if (fclose(temp) != 0 || !copied) {
        remove(temp_path);
        return KV_ERR_IO;
    }

    /* Старый файл закрываем и заменяем новым. На некоторых системах
       rename не перезаписывает существующий файл, поэтому сначала remove. */
    fclose(store->journal); /* старый журнал выбрасывается целиком */
    store->journal = NULL;
    if (remove(store->path) != 0 || rename(temp_path, store->path) != 0) {
        return KV_ERR_IO;
    }
    store->journal = fopen(store->path, "r+b");
    if (store->journal == NULL) {
        return KV_ERR_OPEN;
    }
    return KV_OK;
}

/* Создаёт пустую базу без журнала. Возвращает NULL при нехватке памяти. */
static KvStore *create(const char *path) {
    /* calloc обнуляет все поля: journal и path равны NULL, и kv_close
       можно звать на любом этапе открытия. */
    KvStore *store = calloc(1, sizeof(KvStore));
    if (store == NULL) {
        return NULL;
    }
    store->path = malloc(strlen(path) + 1);
    if (store->path == NULL || !store_init(&store->table)) {
        kv_close(store);
        return NULL;
    }
    strcpy(store->path, path);
    return store;
}

KvStatus kv_open(const char *path, KvStore **out_store) {
    if (path == NULL || out_store == NULL) {
        return KV_ERR_ARG;
    }
    *out_store = NULL;
    KvStore *store = create(path);
    if (store == NULL) {
        return KV_ERR_MEMORY;
    }

    /* Режим "r+b" открывает существующий файл на чтение и запись, не обрезая
       его. Если файла нет, создаём его режимом "w+b". Двоичный режим
       обязателен: текстовый на части систем портит байты перевода строки. */
    store->journal = fopen(path, "r+b");
    if (store->journal == NULL) {
        store->journal = fopen(path, "w+b");
    }
    KvStatus status = store->journal != NULL ? KV_OK : KV_ERR_OPEN;
    long good_end = 0;
    if (status == KV_OK) {
        status = recover(store, &good_end);
    }
    if (status == KV_OK && store->broken > 0) {
        status = cut_tail(store, good_end);
    }
    /* Дописывать будем с конца целых записей. */
    if (status == KV_OK && fseek(store->journal, good_end, SEEK_SET) != 0) {
        status = KV_ERR_IO;
    }
    if (status != KV_OK) {
        kv_close(store); /* закрывает файл и освобождает всё собранное */
        return status;
    }
    *out_store = store;
    return KV_OK;
}

void kv_close(KvStore *store) {
    if (store == NULL) {
        return;
    }
    if (store->journal != NULL) {
        /* Каждая запись уже сброшена на диск в append, поэтому терять
           при закрытии нечего, и результат fclose здесь не меняет исхода. */
        fclose(store->journal);
    }
    store_destroy(&store->table);
    free(store->path);
    free(store);
}

KvStatus kv_put(KvStore *store, const char *key, const char *value) {
    if (store == NULL || key == NULL || value == NULL || key[0] == '\0') {
        return KV_ERR_ARG;
    }
    if (strlen(key) > RECORD_KEY_MAX || strlen(value) > RECORD_VALUE_MAX) {
        return KV_ERR_ARG;
    }
    /* Сначала журнал, потом память: если программа умрёт между ними,
       при следующем открытии запись прочитается из журнала.
       В обратном порядке она пропала бы. */
    KvStatus status = append(store, RECORD_FLAG_PUT, key, value);
    if (status != KV_OK) {
        return status;
    }
    int was_present = 0;
    if (store_put(&store->table, key, value, &was_present) != STORE_OK) {
        return KV_ERR_MEMORY;
    }
    if (was_present) {
        ++store->garbage;
    }
    return KV_OK;
}

KvStatus kv_get(const KvStore *store, const char *key, const char **out_value) {
    if (store == NULL || key == NULL || out_value == NULL) {
        return KV_ERR_ARG;
    }
    if (store_get(&store->table, key, out_value) != STORE_OK) {
        return KV_ERR_NOT_FOUND;
    }
    return KV_OK;
}

KvStatus kv_delete(KvStore *store, const char *key) {
    if (store == NULL || key == NULL) {
        return KV_ERR_ARG;
    }
    const char *value = NULL;
    if (store_get(&store->table, key, &value) != STORE_OK) {
        return KV_ERR_NOT_FOUND;
    }
    /* Надгробие — такая же запись в журнале, только с другим флагом.
       Как и у kv_put, сначала журнал, потом память. */
    KvStatus status = append(store, RECORD_FLAG_TOMBSTONE, key, "");
    if (status != KV_OK) {
        return status;
    }
    store_remove(&store->table, key);
    store->garbage += 2; /* сама запись и то, что она отменяет */
    return KV_OK;
}

int kv_count(const KvStore *store) {
    return store != NULL ? store_count(&store->table) : 0;
}

long kv_journal_records(const KvStore *store) {
    return store != NULL ? store->records : 0;
}

long kv_journal_garbage(const KvStore *store) {
    return store != NULL ? store->garbage : 0;
}

long kv_recovered_broken(const KvStore *store) {
    return store != NULL ? store->broken : 0;
}

void kv_for_each(const KvStore *store,
                 void (*visit)(const char *key, const char *value, void *context),
                 void *context) {
    if (store != NULL) {
        store_for_each(&store->table, visit, context);
    }
}
