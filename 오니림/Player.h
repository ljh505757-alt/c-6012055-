#pragma once

#include "Card.h"
#include <string>
#include <vector>

class Player {
public:
    explicit Player(std::string name);
    const std::string& getName() const;
    const std::vector<Card>& hand() const;
    std::size_t handCount() const;
    void addCard(const Card& card);
    Card takeCard(std::size_t index);
    void clearHand();
    void setHand(const std::vector<Card>& hand);

private:
    std::string name_;
    std::vector<Card> hand_;
};
