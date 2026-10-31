#include "status.h"

const char *img_status_text(ImgStatus status) {
    switch (status) {
    case IMG_OK:
        return "success";
    case IMG_ENOMEM:
        return "not enough memory";
    case IMG_EOPEN:
        return "cannot open the file";
    case IMG_EFORMAT:
        return "invalid format: not a PPM or the header is corrupted";
    case IMG_ECORRUPT:
        return "corrupted data: the file is cut off or a value is out of range";
    case IMG_EARG:
        return "invalid argument";
    case IMG_EWRITE:
        return "write failed";
    default:
        return "unknown error";
    }
}
