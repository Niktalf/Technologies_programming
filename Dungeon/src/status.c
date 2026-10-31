#include "status.h"

const char *status_text(Status status) {
    switch (status) {
    case STATUS_OK:
        return "success";
    case STATUS_NOT_FOUND:
        return "file not found";
    case STATUS_BAD_FORMAT:
        return "corrupted format";
    case STATUS_NO_MEMORY:
        return "not enough memory";
    case STATUS_BAD_SIZE:
        return "invalid size";
    default:
        return "unknown error";
    }
}
