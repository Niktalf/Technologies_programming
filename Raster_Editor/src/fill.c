#include "fill.h"

/* Неизменные на время заливки данные вынесены из параметров рекурсивной
   функции: чем меньше параметров, тем меньше каждый кадр стека
   и тем глубже можно зайти до переполнения. */
static Image *target_image;
static Pixel old_color;
static Pixel new_color;
static FillReport current;

static void fill_step(int x, int y, int depth) {
    /* Условия остановки — в самом начале и в правильном порядке:
       сначала границы, и только потом цвет. Иначе мы сравним цвет
       пикселя, которого нет, то есть прочитаем чужую память. */
    if (!image_inside(target_image, x, y)) {
        return;
    }
    if (image_get(target_image, x, y) != old_color) {
        return; /* чужой цвет или уже перекрашено */
    }
    if (depth > FILL_MAX_DEPTH) {
        current.truncated = 1;
        return;
    }

    image_set(target_image, x, y, new_color);
    ++current.filled;
    if (depth > current.max_depth) {
        current.max_depth = depth;
    }

    fill_step(x + 1, y, depth + 1);
    fill_step(x - 1, y, depth + 1);
    fill_step(x, y + 1, depth + 1);
    fill_step(x, y - 1, depth + 1);
}

FillReport fill_region(Image *image, int x, int y, Pixel color) {
    current.filled = 0;
    current.max_depth = 0;
    current.truncated = 0;

    if (!image_inside(image, x, y)) {
        return current;
    }

    target_image = image;
    old_color = image_get(image, x, y);
    new_color = color;

    /* Заливка тем же цветом, что уже есть, — третье условие остановки,
       о котором забывают чаще всего: перекрашенный пиксель не отличается
       от неперекрашенного, и рекурсия никогда не закончится. */
    if (old_color == new_color) {
        return current;
    }

    fill_step(x, y, 1);
    return current; /* возвращается копия, статическая структура остаётся внутри */
}
