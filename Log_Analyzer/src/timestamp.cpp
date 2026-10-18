#include "timestamp.hpp"

#include <iomanip>
#include <sstream>

namespace {

// Сколько единиц в следующей: упаковка та же, что timestamp_pack в версии на C.
constexpr long long MONTHS_IN_YEAR = 12;
constexpr long long DAYS_IN_MONTH = 31;
constexpr long long HOURS_IN_DAY = 24;
constexpr long long MINUTES_IN_HOUR = 60;
constexpr long long SECONDS_IN_MINUTE = 60;

// Читает count цифр начиная с позиции from. Возвращает false, если
// хоть один символ не цифра.
bool read_digits(const std::string& text, std::size_t from, std::size_t count, int& out) {
    int value = 0;
    bool digits = true;
    for (std::size_t i = from; i < from + count && digits; ++i) {
        digits = text[i] >= '0' && text[i] <= '9';
        value = value * 10 + (text[i] - '0');
    }
    out = value;
    return digits;
}

} // namespace

Timestamp Timestamp::from_parts(int year, int month, int day, int hour, int minute,
                                int second) {
    Timestamp result;
    if (year < 0 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23
        || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return result;   // недействительный
    }
    long long value = year;
    value = value * MONTHS_IN_YEAR + (month - 1);
    value = value * DAYS_IN_MONTH + (day - 1);
    value = value * HOURS_IN_DAY + hour;
    value = value * MINUTES_IN_HOUR + minute;
    value = value * SECONDS_IN_MINUTE + second;
    result.value_ = value;
    return result;
}

Timestamp Timestamp::parse(const std::string& text) {
    // Строго 19 символов с разделителями на своих местах.
    if (text.size() != 19 || text[4] != '-' || text[7] != '-' || text[10] != ' '
        || text[13] != ':' || text[16] != ':') {
        return Timestamp();
    }
    int part[6] = { 0, 0, 0, 0, 0, 0 };
    const std::size_t from[6] = { 0, 5, 8, 11, 14, 17 };
    const std::size_t length[6] = { 4, 2, 2, 2, 2, 2 };
    bool digits = true;
    for (int i = 0; i < 6 && digits; ++i) {
        digits = read_digits(text, from[i], length[i], part[i]);
    }
    if (!digits) {
        return Timestamp();
    }
    return from_parts(part[0], part[1], part[2], part[3], part[4], part[5]);
}

Timestamp Timestamp::parse(const std::string& date, const std::string& time) {
    return parse(date + " " + time);
}

Timestamp Timestamp::from_value(long long value) {
    Timestamp result;
    result.value_ = value;
    return result;
}

int Timestamp::hour() const {
    if (!valid()) {
        return -1;
    }
    return static_cast<int>((value_ / (SECONDS_IN_MINUTE * MINUTES_IN_HOUR)) % HOURS_IN_DAY);
}

std::string Timestamp::text() const {
    long long value = value_;
    long long second = value % SECONDS_IN_MINUTE;
    value /= SECONDS_IN_MINUTE;
    long long minute = value % MINUTES_IN_HOUR;
    value /= MINUTES_IN_HOUR;
    long long hour = value % HOURS_IN_DAY;
    value /= HOURS_IN_DAY;
    long long day = value % DAYS_IN_MONTH + 1;
    value /= DAYS_IN_MONTH;
    long long month = value % MONTHS_IN_YEAR + 1;
    long long year = value / MONTHS_IN_YEAR;

    // Строковый поток собирает текст так же, как std::cout печатает:
    // setw задаёт ширину поля, setfill — чем дополнить слева.
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << year << '-' << std::setw(2) << month
        << '-' << std::setw(2) << day << ' ' << std::setw(2) << hour << ':'
        << std::setw(2) << minute << ':' << std::setw(2) << second;
    return out.str();
}
