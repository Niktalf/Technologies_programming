#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "color.hpp"
#include "image.hpp"

// Практика 12: фильтры — иерархия классов.
//
// Filter — абстрактный базовый класс: методы apply и name чисто
// виртуальные (= 0), и объекта «просто фильтра» создать нельзя. Редактор
// хранит фильтры как указатели на базу и применяет их одним циклом,
// не зная, какой фильтр какой: что сделает apply, решает настоящий тип
// объекта.
//
//   Filter
//   ├── PixelFilter        цвет точки зависит только от её старого цвета
//   │   ├── InvertFilter
//   │   ├── GrayFilter
//   │   ├── BrightnessFilter
//   │   └── ContrastFilter
//   └── BlurFilter         цвет точки зависит от соседей
//
// Фильтр не меняет размеров изображения — это часть договора базового
// класса. Код, который применяет фильтры, на это рассчитывает: например,
// запоминает размер до цепочки и показывает его после.
//
// Вопрос: поворот на 90 градусов — фильтр? Ответ: нет. Поворот меняет
// ширину и высоту местами, то есть нарушает договор Filter. Принцип
// подстановки требует, чтобы наследника можно было использовать везде,
// где ждут базовый класс, и ничего не сломалось. Поворот на месте фильтра
// сломал бы любой код, рассчитывающий на прежний размер: выделение области,
// маску того же размера, координаты, запомненные до цепочки. Поэтому
// поворот, обрезка и масштаб — отдельные операции изображения (Image::rotate),
// а не фильтры. Будь у них общая иерархия, это были бы «преобразования»,
// у которых в договоре прямо сказано: размер может измениться.
class Filter {
public:
    // Виртуальный деструктор. Фильтры удаляются через std::unique_ptr<Filter>,
    // то есть через указатель на базу. Без virtual delete вызвал бы только
    // ~Filter, и деструктор наследника не выполнился бы — неопределённое
    // поведение.
    virtual ~Filter() = default;

    // Применяет фильтр к изображению на месте. Размеры не меняются.
    virtual void apply(Image& image) const = 0;

    // Название с параметрами, например «brightness +40».
    virtual std::string name() const = 0;

protected:
    // Конструктор защищённый: создают фильтры только наследники.
    Filter() = default;
};

// Фильтр, который пересчитывает каждую точку отдельно, по её цвету.
// Обход изображения у всех таких фильтров один и написан здесь, а
// наследник задаёт только, во что превращается один цвет.
//
// Писать результат можно прямо в то же изображение: новая точка ни от
// какой другой не зависит.
class PixelFilter : public Filter {
public:
    void apply(Image& image) const override;

protected:
    virtual Color transform(Color color) const = 0;
};

class InvertFilter : public PixelFilter {
public:
    std::string name() const override { return "invert"; }

protected:
    Color transform(Color color) const override { return color.inverted(); }
};

class GrayFilter : public PixelFilter {
public:
    std::string name() const override { return "gray"; }

protected:
    Color transform(Color color) const override { return color.to_gray(); }
};

// Яркость: к каждому каналу прибавляется delta с насыщением.
class BrightnessFilter : public PixelFilter {
public:
    explicit BrightnessFilter(int delta)
        : delta_(delta) {
    }

    std::string name() const override;

protected:
    Color transform(Color color) const override;

private:
    int delta_;
};

// Контраст: каждый канал отдаляется от середины 128 или приближается
// к ней. percent = 100 — без изменений, больше — контрастнее, меньше — серее,
// 0 — всё становится серым 128.
class ContrastFilter : public PixelFilter {
public:
    // Отрицательный процент перевернул бы изображение — это уже не контраст.
    // Такой фильтр не создаётся: std::invalid_argument.
    explicit ContrastFilter(int percent);

    std::string name() const override;

protected:
    Color transform(Color color) const override;

private:
    int percent_;
};

// Размытие: каждая точка — среднее из квадрата 3 на 3 вокруг неё (у края —
// из тех соседей, что есть).
//
// Чем оно отличается от остальных: новый цвет точки зависит от соседей.
// Если писать результат в то же изображение, к моменту обработки следующей
// точки её левый и верхний соседи уже размыты, и размытие «течёт» вправо
// и вниз. Поэтому соседи читаются из копии исходного изображения, а пишется
// результат в само изображение. По этой же причине BlurFilter не может быть
// PixelFilter: тому достаточно одного цвета.
class BlurFilter : public Filter {
public:
    void apply(Image& image) const override;
    std::string name() const override { return "blur"; }
};

// Фильтр по словам команды начиная с words[position]: invert, gray, blur,
// bright <число>, contrast <процент>. Сдвигает position за прочитанные
// слова. При ошибке печатает, что не так, и возвращает пустой указатель.
//
// Единственное место, где перечислены виды фильтров. Новый фильтр — новый
// класс и строчка здесь; цикл применения не меняется.
std::unique_ptr<Filter> make_filter(const std::vector<std::string>& words,
                                    std::size_t& position);

// Названия известных фильтров — для справки.
const char* filter_names();
