#ifndef DEMOS_H
#define DEMOS_H

/* Демонстрации прошлых практик на внутренних модулях библиотеки.
   Каждая возвращает CLI_OK или CLI_ERROR. */

/* Практика 2: сборка записи журнала, разбор, обнаружение порчи. */
int demo_record(const char *key, const char *value);

/* Практика 3: n поисков через хеш против перебора. */
int demo_bench(long n);

/* Практика 3: удаление из середины цепочки проб. */
int demo_collision(void);

#endif /* DEMOS_H */
