#pragma once

#include <istream>
#include <string>

#include "image.hpp"
#include "status.hpp"

// Разновидность файла PPM: текстовая P3 или двоичная P6.
enum class PpmFormat { text, binary };

// Практика 11: чтение и запись изображений в формате PPM.
//
// На практике 10 этим занимался сам класс Image. У него было две причины
// меняться: как хранить пиксели и как их записывать на диск. Появится
// третий формат — пришлось бы менять класс изображения, хотя пиксели
// хранятся по-прежнему. Теперь формат — забота этого класса, а изображение
// знает только о пикселях.
//
// PpmFile работает с изображением только через открытые методы: создаёт
// его конструктором Image(width, height), пишет пиксели set_pixel, читает
// pixel. Внутрь изображения он не заглядывает, поэтому инвариант
// изображения не может нарушить даже при испорченном файле.
//
// Состояния у класса нет — нет и инварианта. Поэтому методы статические:
// объект PpmFile создавать не нужно, вызов выглядит как PpmFile::read(...).
// Вспомогательные шаги разбора — закрытые: снаружи они никому не нужны.
class PpmFile {
public:
    // Читает файл в image. Если файл не прочитан целиком, image не меняется:
    // пиксели читаются во временное изображение, и оно перемещается в image
    // только при успехе. В format — какая разновидность была в файле.
    static ImageStatus read(const std::string& path, Image& image, PpmFormat& format);

    // Пишет изображение. Пустое изображение не пишется — bad_argument.
    static ImageStatus write(const std::string& path, const Image& image, PpmFormat format);

private:
    static ImageStatus read_header(std::istream& file, PpmFormat& format, int& width,
                                   int& height, int& maximum);
    static ImageStatus read_pixels(std::istream& file, PpmFormat format, int maximum,
                                   Image& image);
};
