#include <stdio.h>
#include <string.h>

#include "record.h"

const char *record_status_text(const RecordStatus status)
{
    switch (status) {
    case RECORD_OK:                 return "success";
    case RECORD_ERR_ARG:            return "invalid argument";
    case RECORD_ERR_TOO_LONG:       return "key or value is too long";
    case RECORD_ERR_BAD_CHECKSUM:   return "checksum mismatch";
    default:                        return "unknown error";
    }
}

void record_pack_header(uint8_t *buffer, const RecordHeader *header)
{
    if (buffer == NULL || header == NULL) {
        return;
    }
    buffer[0] = header->flags;
    buffer[1] = (uint8_t)((header->key_length >> 8) & 0xFFu);
    buffer[2] = (uint8_t)(header->key_length & 0xFFu);
    buffer[3] = (uint8_t)((header->value_length >> 24) & 0xFFu);
    buffer[4] = (uint8_t)((header->value_length >> 16) & 0xFFu);
    buffer[5] = (uint8_t)((header->value_length >> 8) & 0xFFu);
    buffer[6] = (uint8_t)(header->value_length & 0xFFu);
    buffer[7] = header->checksum;
}

void record_unpack_header(const uint8_t *buffer, RecordHeader *header)
{
    if (buffer == NULL || header == NULL) {
        return;
    }
    header->flags = buffer[0];
    header->key_length = (uint16_t)(((uint16_t)buffer[1] << 8) | buffer[2]);
    header->value_length = ((uint32_t)buffer[3] << 24)
                         | ((uint32_t)buffer[4] << 16)
                         | ((uint32_t)buffer[5] << 8)
                         |  (uint32_t)buffer[6];
    header->checksum = buffer[7];
}

uint8_t record_checksum(const uint8_t *data, size_t length)
{
    uint8_t sum = 0;

    if (data == NULL) {
        return 0;
    }

    for (size_t i = 0; i < length; ++i) {
        const uint8_t byte = (i == RECORD_HEADER_SIZE - 1) ? 0u : data[i];
        sum = (uint8_t)(sum * 31u + byte);
    }
    return sum;
}

RecordStatus record_build(uint8_t *buffer, size_t capacity,
                          const uint8_t flags, const char *key, const char *value,
                          size_t *out_size)
{
    if (buffer == NULL || key == NULL || value == NULL || out_size == NULL) {
        return RECORD_ERR_ARG;
    }

    const size_t key_length = strlen(key);
    const size_t value_length = strlen(value);

    if (key_length > RECORD_KEY_MAX || value_length > RECORD_VALUE_MAX) {
        return RECORD_ERR_TOO_LONG;
    }
    const size_t total = RECORD_HEADER_SIZE + key_length + value_length;
    if (total > capacity) {
        return RECORD_ERR_ARG;
    }

    RecordHeader header;
    header.flags = flags;
    header.key_length = (uint16_t)key_length;
    header.value_length = (uint32_t)value_length;
    header.checksum = 0;

    record_pack_header(buffer, &header);
    memcpy(buffer + RECORD_HEADER_SIZE, key, key_length);
    memcpy(buffer + RECORD_HEADER_SIZE + key_length, value, value_length);

    buffer[RECORD_HEADER_SIZE - 1] = record_checksum(buffer, total);

    *out_size = total;
    return RECORD_OK;
}

RecordStatus record_parse(const uint8_t *buffer, size_t size, RecordHeader *header)
{
    if (buffer == NULL || header == NULL || size < RECORD_HEADER_SIZE) {
        return RECORD_ERR_ARG;
    }
    record_unpack_header(buffer, header);

    const size_t expected = RECORD_HEADER_SIZE + header->key_length + header->value_length;
    if (expected != size) {
        return RECORD_ERR_BAD_CHECKSUM;
    }
    if (record_checksum(buffer, size) != header->checksum) {
        return RECORD_ERR_BAD_CHECKSUM;
    }
    return RECORD_OK;
}

uint8_t record_flags_set(const uint8_t flags, const uint8_t flag)
{
    return (uint8_t)(flags | flag);
}

uint8_t record_flags_clear(const uint8_t flags, const uint8_t flag)
{
    return (uint8_t)(flags & (uint8_t)~flag);
}

int record_flags_has(const uint8_t flags, const uint8_t flag)
{
    return (flags & flag) != 0;
}

void record_flags_print(const uint8_t flags)
{
    int printed = 0;

    printf("Flags: 0x%02X (", (unsigned)flags);
    if (record_flags_has(flags, RECORD_FLAG_PUT)) {
        printf("record");
        printed = 1;
    }
    if (record_flags_has(flags, RECORD_FLAG_TOMBSTONE)) {
        printf("%stombstone", printed ? ", " : "");
        printed = 1;
    }
    if (record_flags_has(flags, RECORD_FLAG_COMPRESSED)) {
        printf("%scompressed", printed ? ", " : "");
        printed = 1;
    }
    if (!printed) {
        printf("no");
    }
    printf(")\n");
}
