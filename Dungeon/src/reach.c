#include "map.h"
#include "reach.h"

/* Массив пометок того же размера, что и карта. Пометки нельзя хранить
   в самой карте: проверка не должна портить то, что она проверяет. */
static char visited[MAP_ROWS][MAP_COLS];

/* Итоги последней проверки: сколько клеток обошли и как глубоко зашли. */
static int cells_seen;
static int deepest;

static void clear_marks(void) {
    for (int y = 0; y < MAP_ROWS; ++y) {
        for (int x = 0; x < MAP_COLS; ++x) {
            visited[y][x] = 0;
        }
    }
    cells_seen = 0;
    deepest = 0;
}

/* Рекурсивный обход. Базовых случаев три, и все они проверяются в начале,
   до любого рекурсивного вызова: клетка за границей, клетка-стена, клетка
   уже посещена. Каждый означает «дальше отсюда идти не надо». Порядок
   важен: границы проверяются первыми, иначе visited[y][x] прочитал бы
   чужую память. */
static void visit(int x, int y, int depth) {
    if (!map_inside(x, y)) {
        return;
    }
    if (map_at(x, y) == TILE_WALL) {
        return;
    }
    if (visited[y][x]) {
        return; /* без этой проверки две соседние клетки вызывали бы друг друга вечно */
    }

    visited[y][x] = 1;
    ++cells_seen;
    if (depth > deepest) {
        deepest = depth;
    }

    visit(x + 1, y, depth + 1);
    visit(x - 1, y, depth + 1);
    visit(x, y + 1, depth + 1);
    visit(x, y - 1, depth + 1);
}

int reach_exit(int start_x, int start_y) {
    /* Пометки прошлого запуска стираются. Иначе вторая проверка сразу
       наткнулась бы на «уже посещено» и ничего бы не нашла. */
    clear_marks();
    visit(start_x, start_y, 1);

    for (int y = 0; y < MAP_ROWS; ++y) {
        for (int x = 0; x < MAP_COLS; ++x) {
            if (visited[y][x] && map_at(x, y) == TILE_EXIT) {
                return 1;
            }
        }
    }
    return 0;
}

int reach_last_cells(void) {
    return cells_seen;
}

int reach_last_depth(void) {
    return deepest;
}

/* Опыт с переполнением стека. Поле size x size без стен; рекурсия та же.
   Массив пометок большой — четыре мегабайта, — поэтому он статический:
   в стеке ему не место, а стек и так уйдёт на рекурсию.

   Обход идёт сначала вправо, поэтому он змейкой проходит всё поле,
   ни разу не вернувшись, и глубина рекурсии равна числу клеток.

   Результат опыта (Linux, стек 8 МБ, команда stacktest):
     cmake без типа сборки, без оптимизации:
                    поле 500 — глубина 250 000, работает;
                    поле 520 — Segmentation fault.
     cmake -DCMAKE_BUILD_TYPE=Release, -O3:
                    поле 360 — глубина 129 600, работает;
                    поле 380 — Segmentation fault.
   Отсюда кадр стека одного вызова: около 32 байт без оптимизации и около
   60 байт с -O3. С оптимизацией кадр больше: компилятор встраивает часть
   рекурсивных вызовов друг в друга. В Windows стек 1 МБ, и там поле
   падает уже около 120 на 120. Карта 10 на 30 далека от этой границы,
   но на практике 7 размер карты станет произвольным. */
static char field_visited[REACH_FIELD_MAX][REACH_FIELD_MAX];
static int field_size;
static int field_deepest;

static void visit_field(int x, int y, int depth) {
    if (x < 0 || y < 0 || x >= field_size || y >= field_size) {
        return;
    }
    if (field_visited[y][x]) {
        return;
    }
    field_visited[y][x] = 1;
    if (depth > field_deepest) {
        field_deepest = depth;
    }
    visit_field(x + 1, y, depth + 1);
    visit_field(x - 1, y, depth + 1);
    visit_field(x, y + 1, depth + 1);
    visit_field(x, y - 1, depth + 1);
}

int reach_open_field(int size) {
    if (size < 1 || size > REACH_FIELD_MAX) {
        return 0;
    }
    field_size = size;
    field_deepest = 0;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            field_visited[y][x] = 0;
        }
    }
    visit_field(0, 0, 1);
    return field_deepest;
}
