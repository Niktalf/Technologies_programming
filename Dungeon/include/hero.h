#ifndef HERO_H
#define HERO_H

/* Практика 2: состояния героя как набор битов.

   Все шесть состояний хранятся в одной беззнаковой переменной: каждому
   состоянию отведён свой бит. Маска получается сдвигом единицы влево
   на номер бита. Тип беззнаковый, потому что сдвиг знакового числа
   к старшему биту даёт неопределённое поведение. */

#define STATE_POISONED  (1u << 0) /* отравлен */
#define STATE_BLINDED   (1u << 1) /* ослеплён */
#define STATE_INVISIBLE (1u << 2) /* невидим */
#define STATE_PETRIFIED (1u << 3) /* окаменел */
#define STATE_BURNING   (1u << 4) /* горит */
#define STATE_ASLEEP    (1u << 5) /* спит */

#define STATE_COUNT 6

#endif /* HERO_H */
