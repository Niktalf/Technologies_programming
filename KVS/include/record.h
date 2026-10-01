#ifndef RECORD_H
#define RECORD_H

#include <stdint.h>

#define RECORD_HEADER_SIZE 8
#define RECORD_KEY_MAX     0xFFFFu
#define RECORD_VALUE_MAX   0x00FFFFFFu

#define RECORD_FLAG_PUT       (1u << 0)
#define RECORD_FLAG_TOMBSTONE (1u << 1)
#define RECORD_FLAG_COMPRESSED (1u << 2)

typedef struct {
    uint8_t  flags;
    uint16_t key_length;
    uint32_t value_length;
    uint8_t  checksum;
} RecordHeader;

typedef enum {
    RECORD_OK = 0,
    RECORD_ERR_ARG,
    RECORD_ERR_TOO_LONG,
    RECORD_ERR_BAD_CHECKSUM
} RecordStatus;

const char *record_status_text(RecordStatus status);

void record_pack_header(uint8_t *buffer, const RecordHeader *header);
void record_unpack_header(const uint8_t *buffer, RecordHeader *header);

uint8_t record_checksum(const uint8_t *data, size_t length);

RecordStatus record_build(uint8_t *buffer, size_t capacity,
                          uint8_t flags, const char *key, const char *value,
                          size_t *out_size);

RecordStatus record_parse(const uint8_t *buffer, size_t size, RecordHeader *header);

uint8_t record_flags_set(uint8_t flags, uint8_t flag);
uint8_t record_flags_clear(uint8_t flags, uint8_t flag);
int     record_flags_has(uint8_t flags, uint8_t flag);
void    record_flags_print(uint8_t flags);

#endif // RECORD_H
