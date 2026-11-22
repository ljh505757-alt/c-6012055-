#include "Deck.h"
#include <algorithm>
#include <random>
#include <stdexcept>

Deck::Deck() { reset(); }

void Deck::reset() {
    cards_.clear();
    const Color colors[] = { Color::Red, Color::Blue, Color::Green, Color::Brown };
    const int sunCounts[] = { 9, 8, 7, 6 };
    for (int i = 0; i < 4; ++i) {
        for (int n = 0; n < sunCounts[i]; ++n) cards_.emplace_back(CardType::Path, colors[i], Symbol::Sun);
        for (int n = 0; n < 4; ++n) cards_.emplace_back(CardType::Path, colors[i], Symbol::Moon);
        for (int n = 0; n < 3; ++n) cards_.emplace_back(CardType::Path, colors[i], Symbol::Key);
        for (int n = 0; n < 2; ++n) cards_.emplace_back(CardType::Door, colors[i]);
    }
    for (int i = 0; i < 10; ++i) cards_.emplace_back(CardType::Nightmare);
    shuffle();
}

void Deck::shuffle() {
    static std::mt19937 rng(std::random_device{}());
    std::shuffle(cards_.begin(), cards_.end(), rng);
}

bool Deck::empty() const { return cards_.empty(); }
int Deck::remaining() const { return static_cast<int>(cards_.size()); }

Card Deck::draw() {
    if (cards_.empty()) throw std::out_of_range("덱이 비었습니다.");
    Card card = cards_.back();
    cards_.pop_back();
    return card;
}

bool Deck::takeDoor(Color color, Card& door) {
    for (auto it = cards_.begin(); it != cards_.end(); ++it) {
        if (it->getType() == CardType::Door && it->getColor() == color) {
            door = *it;
            cards_.erase(it);
            shuffle();
            return true;
        }
    }
    return false;
}

std::vector<Card> Deck::revealTop(std::size_t count) {
    std::vector<Card> revealed;
    while (count-- > 0 && !cards_.empty()) revealed.push_back(draw());
    return revealed;
}

void Deck::putOnTop(const std::vector<Card>& cardsInTopOrder) {
    for (auto it = cardsInTopOrder.rbegin(); it != cardsInTopOrder.rend(); ++it) cards_.push_back(*it);
}

void Deck::addAndShuffle(const std::vector<Card>& cards) {
    cards_.insert(cards_.end(), cards.begin(), cards.end());
    shuffle();
}

const std::vector<Card>& Deck::cards() const { return cards_; }

void Deck::setCards(const std::vector<Card>& cards) { cards_ = cards; }
