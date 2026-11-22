#include "Player.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

Player::Player(std::string name)
    : name_(std::move(name)), hand_(5, Card(CardType::Empty)) {}
const std::string& Player::getName() const { return name_; }
const std::vector<Card>& Player::hand() const { return hand_; }

std::size_t Player::handCount() const {
    return static_cast<std::size_t>(std::count_if(hand_.begin(), hand_.end(),
        [](const Card& card) { return card.getType() != CardType::Empty; }));
}

void Player::addCard(const Card& card) {
    for (Card& slot : hand_) {
        if (slot.getType() == CardType::Empty) {
            slot = card;
            return;
        }
    }
    throw std::overflow_error("손패가 가득 찼습니다.");
}

Card Player::takeCard(std::size_t index) {
    if (index >= hand_.size()) throw std::out_of_range("Invalid hand index.");
    if (hand_[index].getType() == CardType::Empty) throw std::out_of_range("빈 손패 칸입니다.");
    Card card = hand_[index];
    hand_[index] = Card(CardType::Empty);
    return card;
}

void Player::clearHand() { std::fill(hand_.begin(), hand_.end(), Card(CardType::Empty)); }

void Player::setHand(const std::vector<Card>& hand) { hand_ = hand; }
