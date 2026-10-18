#pragma once

#include <string>
#include <vector>

#include "image.hpp"

// Редактор: имя файла, рабочее изображение и команды над ним.
//
// На практике 8 это была структура Editor, а функции do_crop, do_rotate
// получали её адрес первым параметром. Здесь обработчики — закрытые методы:
// редактор у них есть всегда — это объект, через который они вызваны.
//
// Кроме рабочего изображения у редактора есть вспомогательное: в него
// пишется результат поворота, масштаба, обрезки и чтения файла, и только
// если операция удалась, оно копируется в рабочее. Так ошибка не портит
// то, что уже открыто, — как в версии на C, где новое изображение
// создавалось раньше, чем освобождалось старое.
class Editor {
public:
    // Выполняет одну команду. words[0] — её название, регистр не важен.
    // Возвращает false, если пора завершать работу.
    bool execute(const std::vector<std::string>& words);

    void set_path(const std::string& path) { path_ = path; }

    // Есть ли изображение и записать его в файл, заданный при запуске.
    bool loaded() const { return loaded_; }
    ImageStatus save_to_path() const { return image_.save(path_, PpmFormat::text); }
    const std::string& path() const { return path_; }

    static void print_help();

private:
    bool require_image() const;
    // Делает так, чтобы в редакторе было изображение width x height.
    bool ensure_image(int width, int height);
    // Переносит готовый результат из вспомогательного изображения в рабочее.
    void take_result();

    void print_info() const;
    void command_size(const std::vector<std::string>& words);
    void command_gen(const std::vector<std::string>& words);
    void command_invert();
    void command_bright(const std::vector<std::string>& words);
    void command_crop(const std::vector<std::string>& words);
    void command_rotate(const std::vector<std::string>& words);
    void command_scale(const std::vector<std::string>& words);
    void command_fill(const std::vector<std::string>& words);
    void command_filltest();
    void command_minmax() const;
    void command_load(const std::vector<std::string>& words);
    void command_save(const std::vector<std::string>& words);
    static void command_colors();

    std::string path_;      // пустая строка — файл не задан
    bool loaded_ = false;   // есть ли в редакторе изображение
    Image image_;           // рабочее изображение
    Image scratch_;         // вспомогательное: сюда пишется результат операции
};
