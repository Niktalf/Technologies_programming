#include "file_reader.hpp"

#include <iostream>
#include <utility>

#include "status.hpp"

bool FileReader::trace = false;
int FileReader::open_count_ = 0;

FileReader::FileReader(const std::string& path)
    : file_(std::fopen(path.c_str(), "r")), path_(path) {
    if (file_ == nullptr) {
        // Исключение из конструктора: объект не считается созданным,
        // и деструктор для него вызван не будет. Поэтому файл, которого
        // нет, и закрывать не придётся.
        throw LogError(LogStatus::cannot_open, path);
    }
    ++open_count_;
    if (trace) {
        std::cout << "  [FileReader] opened " << path_ << '\n';
    }
}

void FileReader::close() noexcept {
    if (file_ == nullptr) {
        return;   // файл забрал другой объект
    }
    std::fclose(file_);
    file_ = nullptr;
    --open_count_;
    if (trace) {
        std::cout << "  [FileReader] closed " << path_ << '\n';
    }
}

FileReader::~FileReader() {
    close();
}

FileReader::FileReader(FileReader&& other) noexcept
    : file_(other.file_), path_(std::move(other.path_)) {
    other.file_ = nullptr;   // прежний владелец больше ничего не закроет
    if (trace) {
        std::cout << "  [FileReader] moved " << path_ << " to a new object\n";
    }
}

FileReader& FileReader::operator=(FileReader&& other) noexcept {
    if (this != &other) {
        close();   // свой файл закрываем, чужой забираем
        file_ = other.file_;
        path_ = std::move(other.path_);
        other.file_ = nullptr;
    }
    return *this;
}

bool FileReader::read_line(std::string& line) {
    line.clear();
    // Строка читается кусками по 255 символов, пока не встретится перевод
    // строки: длина строки ничем не ограничена.
    char chunk[256];
    bool got_something = false;
    bool line_ended = false;
    while (!line_ended && std::fgets(chunk, sizeof chunk, file_) != nullptr) {
        got_something = true;
        line += chunk;
        line_ended = line.back() == '\n';
    }
    // Ошибка чтения отличается от конца файла только флагом потока.
    if (std::ferror(file_) != 0) {
        throw LogError(LogStatus::corrupt, path_);
    }
    if (line_ended) {
        line.pop_back();
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();   // файл из Windows
    }
    return got_something;   // последняя строка без перевода строки — тоже строка
}
