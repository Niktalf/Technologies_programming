#ifndef LOGFILE_H
#define LOGFILE_H

#include "level.h"
#include "status.h"
#include "timestamp.h"

/* Практика 3: журнал хранится в памяти массивом разобранных записей,
   а не перечитывается на каждую команду.

   Практика 7: журнал любого размера. Массив записей растёт по мере чтения,
   вместимость удваивается. Текст сообщения хранится в куче по его настоящей
   длине: он больше не обрезается до 255 символов. Предел теперь один —
   длина строки, которую читает fgets, LOGFILE_LINE_MAX.

   Практика 8: запись и журнал — структуры, функции принимают их по адресу;
   все функции, способные не справиться, возвращают код из status.h. */

#define LOGFILE_LINE_MAX   512   /* длина строки вместе с переводом строки и нулём */
#define LOGFILE_FIRST_CAPACITY 1024 /* вместимость массива после первого роста */
#define LOGFILE_MODULE_MAX 32
#define LOGFILE_PATH_MAX   256
#define LOGFILE_SHOW       10    /* сколько записей печатают head и tail */

/* Одна запись журнала: строка «время|уровень|модуль|текст», разобранная на поля. */
typedef struct {
    Timestamp time;
    Level level;
    char module[LOGFILE_MODULE_MAX];
    char *text; /* в куче, по длине сообщения; принадлежит журналу */
} LogRecord;

typedef struct {
    LogRecord *records; /* растущий массив в куче */
    long capacity;      /* на сколько записей хватает массива */
    long count;         /* сколько записей в массиве */
    long lines;         /* сколько строк в файле */
    long broken;        /* строк, которые не удалось разобрать */
    long no_memory;     /* записей, потерянных из-за нехватки памяти */
    long truncated;     /* строк длиннее буфера чтения */
    int loaded;         /* признак загруженного журнала */
    int sorted;         /* упорядочены ли записи по времени */
    char path[LOGFILE_PATH_MAX];
} LogFile;

/* Сам объект журнала теперь маленький: всё содержимое в куче. */

void logfile_init(LogFile *log);

/* Освобождает тексты всех записей и сам массив. Вызывать дважды безопасно. */
void logfile_destroy(LogFile *log);

/* Добавляет готовую запись, увеличивая массив при необходимости.
   Текст записи переходит во владение журнала. Возвращает LOG_OK
   или LOG_ERR_MEMORY; при неудаче текст остаётся за вызывающим. */
LogStatus logfile_append(LogFile *log, const LogRecord *record);

/* Читает файл целиком в массив записей. Прежнее содержимое освобождается.
   Строки, которые не удалось разобрать, считаются, но не прерывают чтение.
   Если файл не открылся, прежнее содержимое остаётся. */
LogStatus logfile_load(LogFile *log, const char *path);

void logfile_print_record(const LogRecord *record);
/* Первые и последние count записей среди тех, что проходят фильтр mask.
   Фильтр по уровням, введённый на практике 2, действует на все команды вывода. */
void logfile_print_head(const LogFile *log, int count, LevelMask mask);
void logfile_print_tail(const LogFile *log, int count, LevelMask mask);

/* Сколько записей проходит фильтр. */
long logfile_count_matching(const LogFile *log, LevelMask mask);

/* Самое раннее и самое позднее время сразу: два результата возвращаются
   через указатели, признак — обычным значением: 1, если записи есть, 0, если
   журнал пуст. Это ответ, а не ошибка, поэтому код ошибки здесь не нужен.
   Вернуть «нет записей» одним числом нельзя: любое время может быть настоящим. */
int logfile_time_range(const LogFile *log, Timestamp *out_first, Timestamp *out_last);

#endif /* LOGFILE_H */
