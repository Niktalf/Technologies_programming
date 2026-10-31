#include "status.h"

const char *status_text(LogStatus status) {
    switch (status) {
    case LOG_OK:
        return "success";
    case LOG_ERR_MEMORY:
        return "not enough memory";
    case LOG_ERR_OPEN:
        return "cannot open the file";
    case LOG_ERR_FORMAT:
        return "invalid format";
    case LOG_ERR_CORRUPT:
        return "the data is corrupted";
    case LOG_ERR_ARG:
        return "invalid argument";
    case LOG_ERR_WRITE:
        return "write failed";
    default:
        return "unknown error";
    }
}
