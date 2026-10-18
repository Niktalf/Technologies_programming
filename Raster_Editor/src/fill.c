#include <stdlib.h>

#include "fill.h"

/* Точка, которую ещё предстоит проверить. */
typedef struct {
    int x;
    int y;
} Point;

/* Собственный стек точек в куче. */
typedef struct {
    Point *items;
    int count;
    int capacity;
} PointStack;

/* Кладёт точку на стек, при необходимости увеличивая его вдвое.
   Возвращает 0, если памяти не хватило; стек при этом остаётся прежним. */
static int push(PointStack *stack, int x, int y) {
    if (stack->count == stack->capacity) {
        int capacity = stack->capacity > 0 ? stack->capacity * 2 : 1024;
        /* Результат realloc — во временный указатель: при неудаче он вернёт
           NULL, а старый блок останется на месте, и его ещё нужно освободить.
           Присвоить сразу в stack->items значит потерять старый блок. */
        Point *grown = realloc(stack->items, (size_t)capacity * sizeof(Point));
        if (grown == NULL) {
            return 0;
        }
        stack->items = grown;
        stack->capacity = capacity;
    }
    stack->items[stack->count].x = x;
    stack->items[stack->count].y = y;
    ++stack->count;
    return 1;
}

/* Если точка внутри изображения и старого цвета — перекрашивает её
   и кладёт на стек, чтобы потом проверить её соседей. Перекрашивание
   в момент добавления не даёт положить одну точку на стек дважды,
   поэтому на стеке никогда не больше точек, чем в самой области.
   Возвращает 0, если не хватило памяти. */
static int visit(Image *image, PointStack *stack, int x, int y,
                 Pixel old_color, Pixel new_color, FillReport *report) {
    /* Те же условия, что в рекурсивной версии, и в том же порядке:
       сначала границы, потом цвет. */
    if (!image_inside(image, x, y) || image_get(image, x, y) != old_color) {
        return 1;
    }
    image_set(image, x, y, new_color);
    ++report->filled;
    if (!push(stack, x, y)) {
        return 0;
    }
    if (stack->count > report->max_stack) {
        report->max_stack = stack->count;
    }
    return 1;
}

FillReport fill_region(Image *image, int x, int y, Pixel color) {
    FillReport report = { 0, 0, 0 };
    if (image == NULL || !image_inside(image, x, y)) {
        return report;
    }
    Pixel old_color = image_get(image, x, y);
    /* Заливка тем же цветом, что уже есть: перекрашенный пиксель не отличается
       от неперекрашенного, и заливка никогда бы не закончилась. */
    if (old_color == color) {
        return report;
    }

    PointStack stack = { NULL, 0, 0 };
    int ok = visit(image, &stack, x, y, old_color, color, &report);
    while (ok && stack.count > 0) {
        --stack.count;
        Point p = stack.items[stack.count];
        ok = visit(image, &stack, p.x + 1, p.y, old_color, color, &report)
             && visit(image, &stack, p.x - 1, p.y, old_color, color, &report)
             && visit(image, &stack, p.x, p.y + 1, old_color, color, &report)
             && visit(image, &stack, p.x, p.y - 1, old_color, color, &report);
    }
    report.failed = !ok;
    free(stack.items);
    return report;
}
