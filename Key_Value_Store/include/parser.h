#ifndef PARSER_H
#define PARSER_H

/* Практика 6: разбор строки на слова с кавычками и экранированием.

   Ключ или значение может содержать пробелы — тогда его берут в кавычки.
   Сама кавычка внутри значения записывается с обратной косой чертой.

     put "два слова" "он сказал \"да\""

   Разбор — это маленький автомат с тремя состояниями: вне слова,
   внутри кавычек и сразу после обратной косой черты. */

#define PARSER_ARGS_MAX 8

typedef enum {
    PARSER_OK = 0,
    PARSER_ERR_QUOTE,   /* кавычка открыта и не закрыта */
    PARSER_ERR_TOO_MANY /* слов больше, чем помещается */
} ParserStatus;

const char *parser_status_text(ParserStatus status);

/* Режет строку на слова прямо в ней: слова отделяются нулевыми символами,
   указатели на их начала складываются в argv. Строка при этом меняется.
   Число слов возвращается через out_count. */
ParserStatus parser_split(char *line, char *argv[], int max_args, int *out_count);

#endif /* PARSER_H */
