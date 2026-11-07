#include "status.hpp"

const char* status_text(LogStatus status) {
    switch (status) {
    case LogStatus::ok:
        return "success";
    case LogStatus::cannot_open:
        return "cannot open the file";
    case LogStatus::bad_format:
        return "invalid format";
    case LogStatus::corrupt:
        return "the data is corrupted";
    case LogStatus::bad_argument:
        return "invalid argument";
    case LogStatus::write_failed:
        return "write failed";
    }
    return "unknown error";
}
