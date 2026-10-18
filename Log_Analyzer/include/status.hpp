#pragma once

// Результат операции, которая может не получиться. Те же причины, что
// у LogStatus в версии на C, кроме нехватки памяти: строки и векторы
// выделяют память сами, и её нехватку мы пока не обрабатываем.
enum class LogStatus { ok, cannot_open, bad_format, corrupt, bad_argument, write_failed };

// Текст для сообщения — те же слова, что в версии на C.
const char* status_text(LogStatus status);
