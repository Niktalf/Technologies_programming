#include "level_generator.hpp"

#include <cstdlib>

bool LevelGenerator::generate(Map& map, unsigned int seed) {
    bool built = false;
    for (int attempt = 0; attempt < ATTEMPTS && !built; ++attempt) {
        map.clear();   // каждая попытка — с чистой комнаты
        fill(map, seed + static_cast<unsigned int>(attempt));
        built = search_.run(map, 1, 1);
    }
    return built;
}

void LevelGenerator::fill(Map& map, unsigned int seed) const {
    // Генератор тот же, что в версии на C: srand и rand, и случайные числа
    // берутся в том же порядке. Каждое — отдельной инструкцией: порядок
    // вычисления аргументов не определён, и с rand() прямо в аргументах
    // уровень зависел бы от компилятора.
    std::srand(seed);
    int width = map.width();
    int height = map.height();
    int walls = width * height / 25;
    for (int i = 0; i < walls; ++i) {
        int sx = 1 + std::rand() % (width - 2);
        int sy = 1 + std::rand() % (height - 2);
        int length = 2 + std::rand() % 6;
        bool horizontal = std::rand() % 2 != 0;
        for (int k = 0; k < length; ++k) {
            int wx = horizontal ? sx + k : sx;
            int wy = horizontal ? sy : sy + k;
            if (wx > 2 || wy > 2) {   // угол со стартом не застраиваем
                map.set_tile(wx, wy, Map::WALL);
            }
        }
    }
    int piles = width * height / 60 + 1;
    for (int i = 0; i < piles; ++i) {
        int gx = 1 + std::rand() % (width - 2);
        int gy = 1 + std::rand() % (height - 2);
        if (map.tile(gx, gy) == Map::FLOOR) {
            map.set_tile(gx, gy, Map::GOLD);
        }
    }
    map.set_tile(width - 2, height - 2, Map::EXIT);
}
