#include "pixel_ops.hpp"

namespace {

// Единственный двойной цикл по изображению во всём проекте.
void for_each_point(int width, int height, const std::function<void(int x, int y)>& at) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            at(x, y);
        }
    }
}

} // namespace

void for_each_pixel_at(Image& image, const PositionOp& op) {
    // Захват [&]: лямбда живёт только внутри этого вызова, а image и op —
    // дольше. Ссылки не переживут то, на что ссылаются.
    for_each_point(image.width(), image.height(), [&](int x, int y) {
        image.set_pixel(x, y, op(image.pixel(x, y), x, y));
    });
}

void for_each_pixel(Image& image, const PixelOp& op) {
    // Операции без координат координаты не нужны: обёртка их отбрасывает.
    for_each_pixel_at(image, [&op](Color color, int, int) { return op(color); });
}

void visit_pixels(const Image& image, const PixelVisitor& visit) {
    for_each_point(image.width(), image.height(), [&](int x, int y) {
        visit(image.pixel(x, y), x, y);
    });
}
