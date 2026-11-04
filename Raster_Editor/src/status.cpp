#include "../../../../../Technologies_programming/Raster_Editor/include/status.hpp"

const char* status_text(ImageStatus status) {
    switch (status) {
    case ImageStatus::ok:
        return "success";
    case ImageStatus::cannot_open:
        return "cannot open the file";
    case ImageStatus::bad_format:
        return "invalid format: not a PPM or the header is corrupted";
    case ImageStatus::corrupt:
        return "corrupted data: the file is cut off or a value is out of range";
    case ImageStatus::too_large:
        return "too large: at most 8192 x 8192";
    case ImageStatus::bad_argument:
        return "invalid argument";
    case ImageStatus::write_failed:
        return "write failed";
    }
    return "unknown error";
}
