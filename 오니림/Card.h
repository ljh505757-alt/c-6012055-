#pragma once

#include <string>
#include <vector>

enum class CardType { Empty, Path, Door, Nightmare };
enum class Color { Red, Blue, Green, Brown, None };
enum class Symbol { Sun, Moon, Key, None };

class Card {
public:
    Card(CardType type = CardType::Path, Color color = Color::None, Symbol symbol = Symbol::None);

    CardType getType() const;
    Color getColor() const;
    Symbol getSymbol() const;
    std::string name() const;
    std::string shortName() const;
    std::vector<std::string> textArt(int handNumber) const;

private:
    CardType type_;
    Color color_;
    Symbol symbol_;
};

std::string colorName(Color color);
std::string symbolName(Symbol symbol);
