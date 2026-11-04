#pragma once

#include <string>

// Момент времени, упакованный в одно число, как Timestamp в версии на C:
// порядок чисел совпадает с порядком моментов. Месяц всегда считается
// за 31 день: даты не складываются и не вычитаются, а только сравниваются.
//
// На практике 2 упаковка и распаковка были функциями timestamp_pack
// и timestamp_unpack. Здесь число спрятано в класс, и снаружи его можно
// только сравнить, напечатать или узнать час.
//
// Инвариант: value_ == -1 (недействительный момент) или value_ — упакованный
// момент с годом от MIN_YEAR до MAX_YEAR включительно. Поэтому любые два
// действительных момента сравнимы, а text() всегда даёт четыре цифры года.
// Все пути внутрь — from_parts, parse и from_value — проверяют пределы;
// число вне пределов даёт недействительный момент, а не «какой-нибудь».
class Timestamp {
public:
    // Разумные границы года. На практике 10 годился любой неотрицательный
    // год, и from_value из испорченного двоичного файла мог дать год
    // в миллион — text() печатал его семью цифрами.
    static constexpr int MIN_YEAR = 1970;
    static constexpr int MAX_YEAR = 9999;

    // Момент по частям. Если части вне допустимых пределов, результат
    // недействителен: valid() вернёт false.
    static Timestamp from_parts(int year, int month, int day, int hour, int minute, int second);

    // Разбор «ГГГГ-ММ-ДД чч:мм:сс» — одним словом или датой и временем
    // по отдельности, как их дают команды since и between.
    static Timestamp parse(const std::string& text);
    static Timestamp parse(const std::string& date, const std::string& time);

    // Момент по готовому числу — из двоичного файла. Число вне пределов
    // даёт недействительный момент.
    static Timestamp from_value(long long value);

    bool valid() const { return value_ >= 0; }
    long long value() const { return value_; }
    int hour() const;

    // Раньше ли этот момент, чем other.
    bool is_before(Timestamp other) const { return value_ < other.value_; }

    // Обратно в вид «2026-03-15 10:30:00».
    std::string text() const;

private:
    long long value_ = -1;   // -1 — недействительный момент, как TIMESTAMP_INVALID
};
