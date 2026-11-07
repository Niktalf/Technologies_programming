#include "level.hpp"

#include <cctype>
#include <iomanip>
#include <iostream>

namespace {

const char* const NAMES[LEVEL_COUNT] = { "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };

// Имена уровней записаны заглавными, поэтому заглавной делается только
// буква слова.
bool equals_ignore_case(const std::string& word, const std::string& name) {
    if (word.size() != name.size()) {
        return false;
    }
    bool same = true;
    for (std::size_t i = 0; i < word.size() && same; ++i) {
        same = std::toupper(static_cast<unsigned char>(word[i])) == name[i];
    }
    return same;
}

} // namespace

bool level_from_word(const std::string& word, Level& out) {
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        if (equals_ignore_case(word, NAMES[i])) {
            out = static_cast<Level>(i);
            return true;
        }
    }
    return false;
}

const char* level_name(Level level) {
    return NAMES[static_cast<int>(level)];
}

void LevelFilter::allow_all() {
    mask_ = (1u << LEVEL_COUNT) - 1u;
}

void LevelFilter::allow_none() {
    mask_ = 0;
}

void LevelFilter::allow(Level level) {
    mask_ = static_cast<std::uint8_t>(mask_ | bit(level));
}

bool LevelFilter::allows(Level level) const {
    // Результат И — значение бита, а не единица. Сравниваем с нулём.
    return (mask_ & bit(level)) != 0;
}

void LevelFilter::print() const {
    std::cout << "Level filter: ";
    bool printed = false;
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        if (allows(static_cast<Level>(i))) {
            std::cout << (printed ? ", " : "") << NAMES[i];
            printed = true;
        }
    }
    if (!printed) {
        std::cout << "empty (no level passes)";
    }
    // Флаги потока после вывода возвращаются: иначе все следующие числа
    // тоже печатались бы шестнадцатеричными.
    std::ios::fmtflags flags = std::cout.flags();
    char fill = std::cout.fill();
    std::cout << "\nMask: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
              << static_cast<int>(mask_) << '\n';
    std::cout.flags(flags);
    std::cout.fill(fill);
}
