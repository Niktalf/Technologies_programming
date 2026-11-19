#pragma once

#include <functional>
#include <vector>

class Game;
class Monster;

// Практика 14: выборки над монстрами — filter, map, reduce.
//
// Предикат — функция, которая получает монстра и отвечает «да» или «нет».
// Он только отвечает: монстр передан по константной ссылке, и менять его
// предикат не может. Иначе результат выборки зависел бы от того, в каком
// порядке и сколько раз алгоритм вызовет предикат.
using MonsterPredicate = std::function<bool(const Monster& monster)>;

// Предикаты. Каждая функция возвращает лямбду.
MonsterPredicate is_alive();
// Не дальше radius шагов по сетке от героя. game захвачен по ссылке:
// предикат годен, пока жива игра, — то есть в пределах одной команды.
MonsterPredicate near_hero(const Game& game, int radius);
// Оба предиката сразу — конъюнкция без переписывания ни одного из них.
MonsterPredicate both(MonsterPredicate first, MonsterPredicate second);

// Свёртки над выборкой. Начальное значение — 0: нейтральный элемент
// сложения. Тип начального значения — long: он же тип результата.
long total_health(const std::vector<const Monster*>& monsters);
long total_attack(const std::vector<const Monster*>& monsters);

// Отображение: каждый монстр превращается в свой символ на карте.
// Исходная выборка не меняется — строится новый вектор.
std::vector<char> symbols(const std::vector<const Monster*>& monsters);
