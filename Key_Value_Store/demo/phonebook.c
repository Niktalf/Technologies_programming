/* Демонстрационная программа: телефонный справочник поверх библиотеки.

   Она включает только kvstore.h и ничего не знает ни про хеш-таблицу,
   ни про формат журнала, ни про то, как именно данные лежат в файле.
   Если завтра внутренности библиотеки переписать целиком, эту программу
   менять не придётся. */

#include <stdio.h>
#include <string.h>

#include "kvstore.h"

static void print_entry(const char *name, const char *phone, void *context) {
    int *counter = context;
    printf("  %-20s %s\n", name, phone);
    ++*counter;
}

static void usage(const char *program) {
    printf("Phone book.\n");
    printf("  %s <file> add <name> <phone>\n", program);
    printf("  %s <file> find <name>\n", program);
    printf("  %s <file> remove <name>\n", program);
    printf("  %s <file> list\n", program);
}

static int add(KvStore *book, const char *name, const char *phone) {
    KvStatus status = kv_put(book, name, phone);
    if (status != KV_OK) {
        printf("Cannot save: %s\n", kv_status_text(status));
        return 1;
    }
    printf("Saved.\n");
    return 0;
}

static int find(const KvStore *book, const char *name) {
    const char *phone = NULL;
    if (kv_get(book, name, &phone) != KV_OK) {
        printf("No such name.\n");
        return 1;
    }
    printf("%s: %s\n", name, phone);
    return 0;
}

static int remove_entry(KvStore *book, const char *name) {
    KvStatus status = kv_delete(book, name);
    if (status != KV_OK) {
        printf("Cannot delete: %s\n", kv_status_text(status));
        return 1;
    }
    printf("Deleted.\n");
    return 0;
}

static int list(const KvStore *book) {
    int shown = 0;
    kv_for_each(book, print_entry, &shown);
    if (shown == 0) {
        printf("The phone book is empty.\n");
    } else {
        printf("Entries total: %d\n", shown);
    }
    printf("Journal records: %ld, no longer needed among them: %ld\n",
           kv_journal_records(book), kv_journal_garbage(book));
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    KvStore *book = NULL;
    KvStatus status = kv_open(argv[1], &book);
    if (status != KV_OK) {
        printf("Cannot open the phone book: %s\n", kv_status_text(status));
        return 1;
    }
    if (kv_recovered_broken(book) > 0) {
        printf("Warning: the end of the file was damaged and discarded.\n");
    }

    const char *action = argv[2];
    int code = 0;
    if (strcmp(action, "add") == 0 && argc >= 5) {
        code = add(book, argv[3], argv[4]);
    } else if (strcmp(action, "find") == 0 && argc >= 4) {
        code = find(book, argv[3]);
    } else if (strcmp(action, "remove") == 0 && argc >= 4) {
        code = remove_entry(book, argv[3]);
    } else if (strcmp(action, "list") == 0) {
        code = list(book);
    } else {
        usage(argv[0]);
        code = 1;
    }

    kv_close(book);
    return code;
}
