#pragma once

// Результат операции, которая может не получиться. Им отвечают и изображение
// (неверный размер, неверные аргументы обрезки), и чтение и запись файлов.
// Поэтому перечисление лежит в своём заголовке, а не внутри одного из классов.
enum class ImageStatus { ok, cannot_open, bad_format, corrupt, too_large, bad_argument,
                         write_failed };

// Текст для пользователя.
const char* status_text(ImageStatus status);
