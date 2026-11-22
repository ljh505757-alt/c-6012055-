#pragma once

#include "Card.h"
#include <cstddef>
#include <vector>

class Deck {
public:
    Deck();
    void reset();
    void shuffle();
    bool empty() const;
    int remaining() const;
    Card draw();
    bool takeDoor(Color color, Card& door);
    std::vector<Card> revealTop(std::size_t count);
    void putOnTop(const std::vector<Card>& cardsInTopOrder);
    void addAndShuffle(const std::vector<Card>& cards);
    const std::vector<Card>& cards() const;
    void setCards(const std::vector<Card>& cards);

private:
    std::vector<Card> cards_;
};
