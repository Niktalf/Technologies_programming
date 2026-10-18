#include "journal_file.hpp"

#include <cstdio>
#include <iostream>
#include <utility>

#include "store.hpp"

bool JournalFile::trace = false;
int JournalFile::open_count_ = 0;

JournalFile::JournalFile(const std::string& path)
    : path_(path) {
    // Режим "r+b" открывает существующий файл на чтение и запись, не обрезая
    // его. Если файла нет, создаём его режимом "w+b". Двоичный режим
    // обязателен: текстовый на части систем портит байты перевода строки.
    file_ = std::fopen(path.c_str(), "r+b");
    if (file_ == nullptr) {
        file_ = std::fopen(path.c_str(), "w+b");
    }
    if (file_ == nullptr) {
        // Исключение из конструктора: объект не создан, деструктора не будет.
        throw KvError(KvStatus::cannot_open, path);
    }
    ++open_count_;
    if (trace) {
        std::cout << "  [JournalFile] opened " << path_ << '\n';
    }
}

void JournalFile::close() noexcept {
    if (file_ == nullptr) {
        return;
    }
    // Сброс буфера: без него последние операции остались бы в памяти
    // программы и пропали. Результат fflush здесь проверить можно, но
    // сообщить об ошибке исключением нельзя — деструктор может работать
    // как раз во время обработки другого исключения.
    std::fflush(file_);
    std::fclose(file_);
    file_ = nullptr;
    --open_count_;
    if (trace) {
        std::cout << "  [JournalFile] flushed and closed " << path_ << '\n';
    }
}

JournalFile::~JournalFile() {
    close();
}

JournalFile::JournalFile(JournalFile&& other) noexcept
    : file_(other.file_), path_(std::move(other.path_)), good_end_(other.good_end_),
      reading_(other.reading_) {
    other.file_ = nullptr;   // прежний владелец больше ничего не закроет
}

JournalFile& JournalFile::operator=(JournalFile&& other) noexcept {
    if (this != &other) {
        close();   // свой файл закрываем, чужой забираем
        file_ = other.file_;
        path_ = std::move(other.path_);
        good_end_ = other.good_end_;
        reading_ = other.reading_;
        other.file_ = nullptr;
    }
    return *this;
}

void JournalFile::rewind_for_reading() {
    std::fseek(file_, 0, SEEK_SET);
    reading_ = true;
    good_end_ = 0;
}

Record::ReadResult JournalFile::read_next(Record& out) {
    Record::ReadResult result = Record::read(file_, out);
    if (result == Record::ReadResult::record) {
        good_end_ = std::ftell(file_);
    }
    // Ошибка чтения выглядит для fread как конец файла: различаем их
    // через ferror, иначе сбой диска сошёл бы за короткий журнал.
    if (std::ferror(file_) != 0) {
        throw KvError(KvStatus::io_error, path_);
    }
    return result;
}

// Отрезает от журнала повреждённый хвост, как на практиках 8 и 9: целые
// записи копируются во временный файл, и он встаёт на место старого.
// Временный файл — тоже объект JournalFile: его закроет деструктор при
// выходе из блока, в том числе если копирование бросит исключение.
void JournalFile::cut_after_last_record() {
    const std::string temp_path = path_ + ".tmp";
    std::remove(temp_path.c_str());   // остаток прошлой неудачной попытки
    {
        JournalFile temp(temp_path);
        std::fseek(file_, 0, SEEK_SET);
        std::vector<std::uint8_t> chunk(4096);
        long left = good_end_;
        while (left > 0) {
            std::size_t want = left < static_cast<long>(chunk.size())
                                   ? static_cast<std::size_t>(left) : chunk.size();
            if (std::fread(chunk.data(), 1, want, file_) != want) {
                throw KvError(KvStatus::io_error, path_);
            }
            chunk.resize(want);
            temp.append(chunk);
            left -= static_cast<long>(want);
        }
    }   // здесь временный файл сброшен на диск и закрыт

    // Старый файл закрываем и заменяем новым. На некоторых системах rename
    // не перезаписывает существующий файл, поэтому сначала remove.
    close();
    if (std::remove(path_.c_str()) != 0 || std::rename(temp_path.c_str(), path_.c_str()) != 0) {
        throw KvError(KvStatus::io_error, path_);
    }
    file_ = std::fopen(path_.c_str(), "r+b");
    if (file_ == nullptr) {
        throw KvError(KvStatus::cannot_open, path_);
    }
    ++open_count_;
    // Заново открытый файл стоит в начале; дописывать нужно с конца.
    std::fseek(file_, good_end_, SEEK_SET);
    reading_ = false;
}

void JournalFile::append(const std::vector<std::uint8_t>& bytes) {
    // Между чтением и записью в один поток стандарт C требует fseek.
    // Дописываем с конца целых записей.
    if (reading_) {
        std::fseek(file_, good_end_, SEEK_SET);
        reading_ = false;
    }
    // Каждая запись сразу сбрасывается на диск, как на практике 9: без
    // сброса при аварии пропала бы не последняя операция, а всё, что
    // осело в буфере.
    if (std::fwrite(bytes.data(), 1, bytes.size(), file_) != bytes.size()
        || std::fflush(file_) != 0) {
        throw KvError(KvStatus::io_error, path_);
    }
}
