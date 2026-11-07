#pragma once

#include <string>
#include <vector>

// Разбор строки на слова (практика 6). Слово в кавычках может содержать
// пробелы: put name "Ivan Petrov". Обратная косая черта делает следующий
// символ обычным: put quote "he said \"yes\"".
//
// В версии на C разбор шёл прямо в строке: слова писались поверх неё,
// а в массив попадали указатели внутрь. Здесь каждое слово — своя
// std::string, и строку менять не нужно.
class Parser {
public:
    // Разбивает line на слова. Возвращает false, если кавычка не закрыта;
    // слова тогда не определены.
    static bool split(const std::string& line, std::vector<std::string>& words);
};
