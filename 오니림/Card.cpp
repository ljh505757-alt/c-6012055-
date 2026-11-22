#include "Card.h"

namespace {
    std::size_t displayWidth(const std::string& text) {
        std::size_t width = 0;
        for (std::size_t i = 0; i < text.size();) {
            unsigned char ch = static_cast<unsigned char>(text[i]);
            if (ch < 0x80) { ++width; ++i; }
            else if ((ch & 0xF0) == 0xE0) { width += 2; i += 3; }
            else if ((ch & 0xE0) == 0xC0) { ++width; i += 2; }
            else { ++i; }
        }
        return width;
    }

    std::string fitToWidth(const std::string& text, std::size_t width) {
        std::string result;
        std::size_t used = 0;
        for (std::size_t i = 0; i < text.size();) {
            const unsigned char ch = static_cast<unsigned char>(text[i]);
            std::size_t bytes = 1;
            std::size_t columns = 1;
            if ((ch & 0xF0) == 0xE0) { bytes = 3; columns = 2; }
            else if ((ch & 0xE0) == 0xC0) bytes = 2;
            if (used + columns > width) break;
            result.append(text, i, bytes);
            i += bytes;
            used += columns;
        }
        return result;
    }

    std::string center(const std::string& text, std::size_t width) {
        const std::size_t textWidth = displayWidth(text);
        const std::string visible = textWidth <= width ? text : fitToWidth(text, width);
        const std::size_t visibleWidth = displayWidth(visible);
        const std::size_t left = (width - visibleWidth) / 2;
        return std::string(left, ' ') + visible + std::string(width - visibleWidth - left, ' ');
    }

    std::string cardLine(const std::string& text) {
        return "|" + center(text, 14) + "|";
    }
}

Card::Card(CardType type, Color color, Symbol symbol) : type_(type), color_(color), symbol_(symbol) {}
CardType Card::getType() const { return type_; }
Color Card::getColor() const { return color_; }
Symbol Card::getSymbol() const { return symbol_; }

std::string colorName(Color color) {
    switch (color) {
    case Color::Red: return "빨강";
    case Color::Blue: return "파랑";
    case Color::Green: return "초록";
    case Color::Brown: return "갈색";
    default: return "-";
    }
}

std::string symbolName(Symbol symbol) {
    switch (symbol) {
    case Symbol::Sun: return "태양";
    case Symbol::Moon: return "달";
    case Symbol::Key: return "열쇠";
    default: return "-";
    }
}

std::string Card::name() const {
    if (type_ == CardType::Empty) return "[ 빈 자리 ]";
    if (type_ == CardType::Door) return "[ 문: " + colorName(color_) + " ]";
    if (type_ == CardType::Nightmare) return "[ 악몽 ]";
    return "[ " + colorName(color_) + " / " + symbolName(symbol_) + " ]";
}

std::string Card::shortName() const {
    if (type_ == CardType::Empty) return "빈 자리";
    if (type_ == CardType::Door) return "문-" + colorName(color_);
    if (type_ == CardType::Nightmare) return "악몽";
    return colorName(color_) + "-" + symbolName(symbol_);
}

std::vector<std::string> Card::textArt(int handNumber) const {
    std::vector<std::string> lines;
    lines.push_back("+--------------+");
    lines.push_back(cardLine("손패 " + std::to_string(handNumber)));

    if (type_ == CardType::Empty) {
        lines.push_back(cardLine("빈 슬롯"));
        lines.push_back(cardLine("  .--------."));
        lines.push_back(cardLine("  |        |"));
        lines.push_back(cardLine("  | 새 카드 |"));
        lines.push_back(cardLine("  | 들어옴 |"));
        lines.push_back(cardLine("  '--------'"));
    } else if (type_ == CardType::Nightmare) {
        lines.push_back(cardLine("악몽 카드"));
        lines.push_back(cardLine("\\  .----.  /"));
        lines.push_back(cardLine(" \\ ( 으악 ) /"));
        lines.push_back(cardLine("  `------'  "));
        lines.push_back(cardLine("꿈을 삼킨다"));
    } else if (type_ == CardType::Door) {
        lines.push_back(cardLine("문 카드"));
        lines.push_back(cardLine("   ||    ||"));
        lines.push_back(cardLine("   || 문 ||"));
        lines.push_back(cardLine("   ||____||"));
        lines.push_back(cardLine(colorName(color_)));
        lines.push_back(cardLine("열쇠를 찾아라"));
    } else {
        lines.push_back(cardLine("미로 카드"));
        lines.push_back(cardLine("  .--------."));
        if (symbol_ == Symbol::Sun) {
            lines.push_back(cardLine("    \\ | /"));
            lines.push_back(cardLine("   -- 해 --"));
            lines.push_back(cardLine("    / | \\"));
        } else if (symbol_ == Symbol::Moon) {
            lines.push_back(cardLine("    .---."));
            lines.push_back(cardLine("   ( 달 )"));
            lines.push_back(cardLine("    `---'"));
        } else {
            lines.push_back(cardLine("      o====+"));
            lines.push_back(cardLine("           ||"));
            lines.push_back(cardLine("      열쇠  ||"));
        }
        lines.push_back(cardLine(colorName(color_)));
        lines.push_back(cardLine(symbolName(symbol_)));
    }

    while (lines.size() < 9) lines.push_back(cardLine(""));
    lines.push_back("+--------------+");
    return lines;
}
