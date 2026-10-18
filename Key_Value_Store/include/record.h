#ifndef RECORD_H
#define RECORD_H

#include <stddef.h>
#include <stdint.h>

/* Практика 2: формат одной записи журнала.
   Запись состоит из заголовка фиксированного размера и следующих за ним
   байтов ключа и значения. Журнал на диске из таких записей — практика 8. */

#define RECORD_HEADER_SIZE 8
/* Практика 8: пределы, которые библиотека соблюдает при записи и проверяет
   при чтении журнала. Поле длины ключа вмещает 65 535, длины значения —
   больше четырёх миллиардов, но такие записи библиотека не пишет: так одна
   запись всегда помещается в буфер на стеке, а испорченная длина в журнале
   отвергается сразу, не заставляя читать мегабайты. */
#define RECORD_KEY_MAX     255u
#define RECORD_VALUE_MAX   4096u
#define RECORD_MAX_SIZE    (RECORD_HEADER_SIZE + RECORD_KEY_MAX + RECORD_VALUE_MAX)

/* Тип операции хранится битами: обычная запись, надгробие (удаление),
   признак сжатого значения на будущее. */
#define RECORD_FLAG_PUT        (1u << 0)
#define RECORD_FLAG_TOMBSTONE  (1u << 1)
#define RECORD_FLAG_COMPRESSED (1u << 2)

typedef struct {
    uint8_t flags;
    uint16_t key_length;
    uint32_t value_length;
    uint8_t checksum;
} RecordHeader;

typedef enum {
    RECORD_OK = 0,
    RECORD_ERR_ARG,
    RECORD_ERR_TOO_LONG,
    RECORD_ERR_BAD_CHECKSUM
} RecordStatus;

const char *record_status_text(RecordStatus status);

/* Раскладывает заголовок по байтам вручную: структура целиком в файл не пишется,
   потому что компилятор вставляет между полями выравнивающие пропуски. */
void record_pack_header(uint8_t *buffer, const RecordHeader *header);
void record_unpack_header(const uint8_t *buffer, RecordHeader *header);

/* Контрольная сумма по всем байтам, включая заголовок с нулевым полем суммы:
   порча длины опаснее порчи данных, поэтому она тоже должна покрываться. */
uint8_t record_checksum(const uint8_t *data, size_t length);

/* Собирает запись целиком в буфер. Возвращает через out_size её полный размер. */
RecordStatus record_build(uint8_t *buffer, size_t capacity,
                          uint8_t flags, const char *key, const char *value,
                          size_t *out_size);

/* Разбирает запись и проверяет её целостность. Сначала сверяет длину из заголовка
   с фактическим размером, потом считает контрольную сумму: по длине мы будем читать
   файл, поэтому испорченная длина опаснее испорченных данных. */
RecordStatus record_parse(const uint8_t *buffer, size_t size, RecordHeader *header);

/* Операции над флагами. */
uint8_t record_flags_set(uint8_t flags, uint8_t flag);
uint8_t record_flags_clear(uint8_t flags, uint8_t flag);
int record_flags_has(uint8_t flags, uint8_t flag);
void record_flags_print(uint8_t flags);

#endif /* RECORD_H */
