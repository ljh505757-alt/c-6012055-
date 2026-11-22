#pragma once

#include "Deck.h"
#include "Player.h"
#include <string>
#include <vector>

class GameManager {
public:
    GameManager();
    void run();

private:
    Deck deck_;
    Player player_;
    std::vector<Card> path_;
    std::vector<Card> gainedDoors_;
    std::vector<Card> discardPile_;
    std::vector<Card> limbo_;
    int turn_;
    bool gameOver_;
    bool victory_;
    bool paused_;

    void resetGame();
    void showTitle() const;
    void showRules() const;
    void showTutorial() const;
    void showBoard() const;
    void showPiles() const;
    void showHand() const;
    void dealHand();
    void playTurn();
    void takeAction();
    void playPath(std::size_t index);
    void discardCard(std::size_t index);
    void prophecy();
    void fillHand();
    void drawLocationForHand();
    void resolveNightmare();
    void acquireDoor(Color color, const std::string& reason);
    void shuffleLimbo();
    void checkDoor();
    bool hasDoor(Color color) const;
    int doorCount(Color color) const;
    int matchingKeyIndex(Color color) const;
    bool canPlay(std::size_t index) const;
    bool isVictory() const;
    int readChoice(const std::string& prompt, int low, int high) const;
    bool saveGame() const;
    bool loadGame();
    bool hasSave() const;
};
