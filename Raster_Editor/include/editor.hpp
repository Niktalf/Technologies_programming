#pragma once

#include <memory>
#include <string>
#include <vector>

#include "filter.hpp"
#include "image.hpp"
#include "ppm_file.hpp"

// Редактор: имя файла, рабочее изображение и команды над ним.
//
// На практике 8 это была структура Editor, а функции do_crop, do_rotate
// получали её адрес первым параметром. Здесь обработчики — закрытые методы:
// редактор у них есть всегда — это объект, через который они вызваны.
//
// Инвариант: image_ — пустое изображение или изображение, удовлетворяющее
// инварианту Image; path_ — любой путь, пустая строка значит «файл
// не задан». Своих проверок у редактора нет: всё, что он делает
// с изображением, идёт через открытые методы Image и PpmFile, и инвариант
// изображения держат они.
//
// Практика 10: изображение само владеет памятью, поэтому редактор маленький
// и живёт обычной локальной переменной. Вспомогательное изображение
// практики 9 больше не нужно: каждая операция строит результат в новом
// объекте и перемещает его на место старого.
class Editor {
public:
    // Выполняет одну команду. words[0] — её название, регистр не важен.
    // Возвращает false, если пора завершать работу.
    bool execute(const std::vector<std::string>& words);

    void set_path(const std::string& path) { path_ = path; }

    // Есть ли изображение и записать его в файл, заданный при запуске.
    bool loaded() const { return !image_.empty(); }
    ImageStatus save_to_path() const { return PpmFile::write(path_, image_, PpmFormat::text); }
    const std::string& path() const { return path_; }

    static void print_help();

private:
    bool require_image() const;
    // Делает так, чтобы в редакторе было изображение width x height.
    bool ensure_image(int width, int height);

    void print_info() const;
    void command_size(const std::vector<std::string>& words);
    void command_gen(const std::vector<std::string>& words);
    void command_invert();
    void command_bright(const std::vector<std::string>& words);
    void command_single_filter(const std::vector<std::string>& words);
    void command_filters(const std::vector<std::string>& words);
    void apply_chain(const std::vector<std::unique_ptr<Filter>>& chain);
    void command_crop(const std::vector<std::string>& words);
    void command_rotate(const std::vector<std::string>& words);
    void command_scale(const std::vector<std::string>& words);
    void command_fill(const std::vector<std::string>& words);
    void command_filltest();
    void command_minmax() const;
    void command_analyze(const std::vector<std::string>& words) const;
    void command_levels();
    void command_load(const std::vector<std::string>& words);
    void command_save(const std::vector<std::string>& words);
    static void command_colors();
    static void command_demo();
    static void try_to_break();
    static void command_stress(const std::vector<std::string>& words);

    std::string path_;   // пустая строка — файл не задан
    Image image_;        // пустое, пока изображения нет
};
