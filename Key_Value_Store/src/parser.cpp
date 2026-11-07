#include "parser.hpp"

#include <cctype>

namespace {

bool is_space(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

} // namespace

bool Parser::split(const std::string& line, std::vector<std::string>& words) {
    words.clear();
    std::size_t i = 0;
    bool closed = true;
    while (i < line.size() && closed) {
        // Пропускаем пробелы между словами.
        while (i < line.size() && is_space(line[i])) {
            ++i;
        }
        if (i < line.size()) {
            std::string word;
            bool in_quotes = false;
            while (i < line.size() && (in_quotes || !is_space(line[i]))) {
                if (line[i] == '\\' && i + 1 < line.size()) {
                    // Обратная косая черта: следующий символ — обычный.
                    word += line[i + 1];
                    i += 2;
                } else if (line[i] == '"') {
                    in_quotes = !in_quotes;
                    ++i;
                } else {
                    word += line[i];
                    ++i;
                }
            }
            closed = !in_quotes;
            words.push_back(word);
        }
    }
    return closed;
}
