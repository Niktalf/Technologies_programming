#include "log_source.hpp"

#include <cstdlib>
#include <utility>

#include "binary_log.hpp"
#include "log_generator.hpp"
#include "log_parser.hpp"
#include "status.hpp"

std::unique_ptr<LogRecord> BookSource::next() {
    if (index_ >= book_.count()) {
        return nullptr;
    }
    // Запись копируется: источник отдаёт записи во владение, а журнал
    // свои записи никому не отдаёт.
    std::unique_ptr<LogRecord> record = std::make_unique<LogRecord>(book_.at(index_));
    ++index_;
    return record;
}

std::string BookSource::describe() const {
    return "loaded log " + book_.path();
}

TextFileSource::TextFileSource(const std::string& path)
    : reader_(path) {
}

std::unique_ptr<LogRecord> TextFileSource::next() {
    std::unique_ptr<LogRecord> record;
    std::string line;
    // Читаем, пока не найдётся верная строка или не кончится файл.
    // Пустые строки пропускаются, испорченные — считаются и пропускаются.
    while (!record && reader_.read_line(line)) {
        ++lines_;
        if (!line.empty()) {
            record = LogParser::parse_line(line);
            if (!record) {
                ++broken_;
            }
        }
    }
    return record;
}

std::string TextFileSource::describe() const {
    return "text " + reader_.path() + ", lines read " + std::to_string(lines_) + ", broken "
           + std::to_string(broken_);
}

BinaryFileSource::BinaryFileSource(const std::string& path)
    : path_(path), file_(path, std::ios::binary) {
    if (!file_) {
        throw LogError(LogStatus::cannot_open, path);
    }
    count_ = BinaryLog::read_header(file_, path);
}

std::unique_ptr<LogRecord> BinaryFileSource::next() {
    if (read_ == count_) {
        return nullptr;
    }
    std::unique_ptr<LogRecord> record = BinaryLog::read_record(file_);
    if (!record) {
        // Заголовок обещал больше записей, чем есть: файл испорчен. Молча
        // закончить нельзя — отчёт выглядел бы верным, а он неполный.
        throw LogError(LogStatus::corrupt, path_);
    }
    ++read_;
    return record;
}

std::string BinaryFileSource::describe() const {
    return "binary " + path_;
}

void MultiFileSource::add(std::unique_ptr<LogSource> source) {
    if (source) {
        sources_.push_back(std::move(source));
    }
}

std::unique_ptr<LogRecord> MultiFileSource::next() {
    std::unique_ptr<LogRecord> record;
    // Кончился текущий источник — переходим к следующему. Каждый из них
    // может быть любым: текстовым, двоичным, даже другим MultiFileSource.
    while (!record && current_ < sources_.size()) {
        record = sources_[current_]->next();
        if (!record) {
            ++current_;
        }
    }
    return record;
}

std::string MultiFileSource::describe() const {
    std::string text = std::to_string(sources_.size()) + " sources:";
    for (const std::unique_ptr<LogSource>& source : sources_) {
        text += " [" + source->describe() + "]";
    }
    return text;
}

RandomSource::RandomSource(long count, unsigned int seed)
    : count_(count), seed_(seed) {
    std::srand(seed);   // та же последовательность, что у gen с этим зерном
}

std::unique_ptr<LogRecord> RandomSource::next() {
    if (produced_ >= count_) {
        return nullptr;
    }
    ++produced_;
    return std::make_unique<LogRecord>(LogGenerator::make_record(produced_));
}

std::string RandomSource::describe() const {
    return "random, " + std::to_string(count_) + " records, seed " + std::to_string(seed_);
}

std::unique_ptr<LogSource> open_file_source(const std::string& path) {
    if (BinaryLog::is_binary(path)) {
        return std::make_unique<BinaryFileSource>(path);
    }
    return std::make_unique<TextFileSource>(path);
}
