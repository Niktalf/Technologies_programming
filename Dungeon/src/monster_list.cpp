#include "monster_list.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>

#include "game.hpp"

namespace {

// Сколько монстров показывает print: длинный список не нужен.
constexpr int SHOWN = 20;

} // namespace

bool MonsterList::add(std::unique_ptr<Monster> monster) {
    if (!monster || find(monster->x(), monster->y()) >= 0) {
        return false;   // monster уничтожится сам при выходе из метода
    }
    // unique_ptr нельзя скопировать — только переместить: владелец у монстра
    // всегда один.
    monsters_.push_back(std::move(monster));
    return true;
}

// Практика 14: поиск — std::find_if вместо цикла с ранним выходом.
int MonsterList::find(int x, int y) const {
    auto found = std::find_if(monsters_.begin(), monsters_.end(),
                              [x, y](const std::unique_ptr<Monster>& monster) {
                                  return monster->x() == x && monster->y() == y;
                              });
    return found == monsters_.end() ? -1 : static_cast<int>(found - monsters_.begin());
}

std::vector<const Monster*> MonsterList::select(const MonsterPredicate& keep) const {
    // Сначала map: из владеющих указателей — простые указатели только для
    // чтения. Потом filter: std::copy_if переписывает подходящие.
    std::vector<const Monster*> all(monsters_.size());
    std::transform(monsters_.begin(), monsters_.end(), all.begin(),
                   [](const std::unique_ptr<Monster>& monster) {
                       return static_cast<const Monster*>(monster.get());
                   });
    std::vector<const Monster*> chosen;
    std::copy_if(all.begin(), all.end(), std::back_inserter(chosen),
                 [&keep](const Monster* monster) { return keep(*monster); });
    return chosen;
}

int MonsterList::hit(int index, int damage) {
    if (index < 0 || index >= count() || damage < 0) {
        return -1;
    }
    Monster& target = *monsters_[static_cast<std::size_t>(index)];
    target.take_damage(damage);
    if (target.alive()) {
        return target.hp();
    }
    // Мёртвого — прочь: на его место перемещается последний указатель,
    // а последний элемент удаляется. Монстр, которым владел удалённый
    // указатель, уничтожается через виртуальный деструктор.
    monsters_[static_cast<std::size_t>(index)] = std::move(monsters_.back());
    monsters_.pop_back();
    return 0;
}

void MonsterList::act_all(Game& game) {
    // Один цикл на всех — без единой проверки вида. Ветвление по видам
    // спрятано в таблице виртуальных функций: вызов act находит метод
    // настоящего класса объекта.
    for (std::size_t i = 0; i < monsters_.size() && game.hero().alive(); ++i) {
        monsters_[i]->act(game);
    }
}

void MonsterList::set_rule_all(const MoveRule& rule) {
    // Каждый монстр получает свою копию правила: std::function копируется
    // вместе со всем, что захватила лямбда.
    for (std::unique_ptr<Monster>& monster : monsters_) {
        monster->set_rule(rule);
    }
}

void MonsterList::make_all_drunk() {
    // drunk получает текущее правило монстра и возвращает новое. У каждого
    // монстра — своё: пьяный гоблин гонится за героем, пьяная мышь порхает.
    for (std::unique_ptr<Monster>& monster : monsters_) {
        monster->set_rule(drunk(monster->rule()));
    }
}

void MonsterList::reset_rules() {
    for (std::unique_ptr<Monster>& monster : monsters_) {
        monster->reset_rule();
    }
}

void MonsterList::print() const {
    std::cout << "Monsters: " << count() << '\n';
    for (int i = 0; i < count() && i < SHOWN; ++i) {
        const Monster& monster = at(i);
        std::cout << "  " << i + 1 << ". " << monster.name() << " (" << monster.x() << ", "
                  << monster.y() << "), health " << monster.hp() << " / " << monster.hp_max()
                  << '\n';
    }
    if (count() > SHOWN) {
        std::cout << "  ... and " << count() - SHOWN << " more\n";
    }
    std::cout << "Monsters created since the start: " << Monster::created_count() << '\n';
}
