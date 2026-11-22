#include "Game.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <windows.h>

namespace {
    constexpr WORD ThemeBackground = 0;
    constexpr WORD ThemeDefault = FOREGROUND_RED | FOREGROUND_GREEN |
        FOREGROUND_BLUE | FOREGROUND_INTENSITY;

    std::size_t terminalWidth(const std::string& text) {
        std::size_t width = 0;
        for (std::size_t i = 0; i < text.size();) {
            const unsigned char ch = static_cast<unsigned char>(text[i]);
            if (ch < 0x80) { ++width; ++i; }
            else if ((ch & 0xF0) == 0xE0) { width += 2; i += 3; }
            else if ((ch & 0xE0) == 0xC0) { ++width; i += 2; }
            else { ++i; }
        }
        return width;
    }

    std::string framedLine(const std::string& text, std::size_t width) {
        const std::size_t textWidth = terminalWidth(text);
        const std::size_t used = (std::min)(textWidth, width);
        const std::string visible = textWidth <= width ? text : text.substr(0, width);
        return "| " + visible + std::string(width - used, ' ') + " |";
    }

    std::string frameBorder(std::size_t width) {
        return "+" + std::string(width + 2, '-') + "+";
    }

    WORD defaultConsoleColor() { return ThemeDefault; }

    void setConsoleColor(Color color) {
        WORD attribute = FOREGROUND_INTENSITY | ThemeBackground;
        switch (color) {
        case Color::Red: attribute |= FOREGROUND_RED; break;
        case Color::Blue: attribute |= FOREGROUND_BLUE; break;
        case Color::Green: attribute |= FOREGROUND_GREEN; break;
        case Color::Brown: attribute |= FOREGROUND_RED | FOREGROUND_GREEN; break;
        default: attribute = defaultConsoleColor(); break;
        }
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attribute);
    }

    void setCardColor(const Card& card) {
        if (card.getType() == CardType::Nightmare) {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY | ThemeBackground);
        } else {
            setConsoleColor(card.getColor());
        }
    }

    const char* doorMarker(bool acquired) {
        return acquired ? "□" : "■";
    }

    void resetConsoleColor() {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ThemeDefault);
    }

    void clearConsole() {
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (!GetConsoleScreenBufferInfo(console, &info)) return;

        const DWORD cellCount = static_cast<DWORD>(info.dwSize.X) * static_cast<DWORD>(info.dwSize.Y);
        const COORD home = { 0, 0 };
        DWORD written = 0;
        FillConsoleOutputCharacterA(console, ' ', cellCount, home, &written);
        FillConsoleOutputAttribute(console, ThemeDefault, cellCount, home, &written);
        SetConsoleCursorPosition(console, home);
    }

    void waitForEnter(const std::string& prompt) {
        std::cout << prompt;
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        std::cin.get();
    }
}

GameManager::GameManager()
    : player_("꿈꾸는 자"), turn_(1), gameOver_(false), victory_(false), paused_(false) {}

void GameManager::resetGame() {
    deck_.reset();
    player_ = Player("꿈꾸는 자");
    path_.clear();
    gainedDoors_.clear();
    discardPile_.clear();
    limbo_.clear();
    turn_ = 1;
    gameOver_ = false;
    victory_ = false;
    paused_ = false;
}

void GameManager::showTitle() const {
    resetConsoleColor();
    const WORD blueGlow = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    const WORD purpleGlow = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    const WORD ghostWhite = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    const std::vector<std::string> blueArt = {
        "      +------------------------------------------+",
        "      |                                          |",
        "      |              .----------.                |",
        "      |              |  .----.  |                |",
        "      |              | ( o  o ) |                |",
        "      |              |    ^^    |                |"
    };
    const std::vector<std::string> whiteArt = {
        "      |              |  .----.  |                |",
        "      |              |  |    |  |                |",
        "      |              |__|    |__|                |",
        "      |                 |    |                   |"
    };
    const std::vector<std::string> mazeArt = {
        "      |       +-------+      +-------+           |",
        "      |   +---+       +------+       +---+       |",
        "      |   |       +------+      +------+ |       |",
        "      +---+_______|      |______|      |+--------+",
        "      |                 +------+      +------+   |"
    };
    std::cout << "\n";
    SetConsoleTextAttribute(console, blueGlow);
    for (const std::string& line : blueArt) std::cout << line << "\n";
    SetConsoleTextAttribute(console, ghostWhite);
    for (const std::string& line : whiteArt) std::cout << line << "\n";
    SetConsoleTextAttribute(console, purpleGlow);
    for (const std::string& line : mazeArt) std::cout << line << "\n";

    SetConsoleTextAttribute(console, blueGlow);
    std::cout << "\n                  O N I R I M\n";
    SetConsoleTextAttribute(console, ghostWhite);
    std::cout << "                   꿈의 미로\n";
    SetConsoleTextAttribute(console, purpleGlow);
    std::cout << "            여덟 개의 문, 하나의 탈출구\n";
    resetConsoleColor();
    std::cout << "\n  이어하기는 턴마다 자동 저장된 진행을 불러옵니다.\n\n";
}
void GameManager::showRules() const {
    resetConsoleColor();
    std::cout << "\n"
        << "  +==========================================================+\n"
        << "  |                       게임 규칙                          |\n"
        << "  +==========================================================+\n"
        << "\n  [게임 목표]\n"
        << "  덱이 바닥나기 전에 네 색의 문을 2장씩, 총 8장 모으세요.\n"
        << "\n  [카드 구성: 총 76장]\n"
        << "  미로 카드 58장 (빨강 16, 파랑 15, 초록 14, 갈색 13)\n"
        << "  문 카드 8장 (색상별 2장), 악몽 카드 10장\n"
        << "\n  [한 턴 진행]\n"
        << "  1. 손패에서 미로 카드 1장을 미로에 내거나 버립니다.\n"
        << "  2. 덱에서 뽑아 손패를 5장까지 채웁니다. 뽑은 문이나 악몽은\n"
        << "     즉시 처리합니다.\n"
        << "  3. 턴 중 림보에 놓인 카드를 덱에 다시 섞습니다.\n"
        << "\n  [길과 문]\n"
        << "  직전 길 카드와 같은 문양(해/달/열쇠)은 연속해서 낼 수 없습니다.\n"
        << "  같은 색 길 카드 3장이 연속되면 덱에서 그 색 문 1장을 얻습니다.\n"
        << "  같은 색 문을 뽑았을 때 손에 든 열쇠를 쓰면 그 문을 얻습니다.\n"
        << "  열쇠 카드를 버리면 예언을 합니다: 덱 위 최대 5장을 보고 1장을\n"
        << "  버린 뒤, 나머지를 원하는 순서로 덱 위에 놓습니다.\n"
        << "\n  [악몽 카드: 아래 중 하나를 선택]\n"
        << "  1) 손패의 열쇠 1장 버리기   2) 얻은 문 1장을 림보로 보내기\n"
        << "  3) 덱 위 최대 5장 공개: 미로 카드는 버리고, 문/악몽은 림보로\n"
        << "  4) 손패 전체를 버리고 새 손패 5장 받기\n"
        << "\n  [문양 및 색 테마]\n"
        << "  해·달은 길 카드 문양, 열쇠는 예언과 같은 색 문을 얻을 때 씁니다.\n"
        << "  빨강: 천문대  |  파랑: 수족관  |  초록: 정원  |  갈색: 도서관\n"
        << "  문 기호: ■는 아직 닫힌 문, □는 획득한 문입니다. 기호 색으로 문 색을 구분합니다.\n"
        << "\n  [입력 방법]\n"
        << "  화면에 표시된 번호를 입력하세요. 카드 고르기 중 0을 입력하면\n"
        << "  행동 메뉴로 돌아가며, 카드를 확정하기 전에 다시 고를 수 있습니다.\n"
        << "\n  계속하려면 메뉴로 돌아가세요.\n";
}

void GameManager::showTutorial() const {
    resetConsoleColor();
    std::cout << "\n  +================ 초보자 튜토리얼 ================+\n"
        << "  | 1. 새 게임을 고르면 카드 5장을 받고 시작합니다.       |\n"
        << "  | 2. 매 턴 길 카드 1장을 내거나 버리고 손패를 채웁니다.|\n"
        << "  | 3. 같은 색, 서로 다른 문양의 길 3장을 이어 보세요.   |\n"
        << "  | 4. 같은 색 문을 얻거나 열쇠로 뽑은 문을 엽니다.      |\n"
        << "  | 5. 악몽이 나오면 선택지 중 페널티를 고릅니다.        |\n"
        << "  | 6. 문 8장을 모으면 승리, 덱이 바닥나면 패배합니다.   |\n"
        << "  +==================================================+\n"
        << "\n  플레이 중 [4] 저장하고 타이틀로를 고르면 진행을 저장합니다.\n"
        << "  계속하려면 Enter를 누르세요...";
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    std::cin.get();
}

void GameManager::showBoard() const {
    resetConsoleColor();
    const Color colors[] = { Color::Red, Color::Blue, Color::Green, Color::Brown };
    std::cout << "\n  ================ 꿈의 미로 ================\n";
    std::cout << "  " << turn_ << "번째 턴 | 덱 " << deck_.remaining()
        << "장 | 손패 " << player_.handCount() << "/5"
        << " | 획득한 문 " << gainedDoors_.size() << "/8\n";
    std::cout << "  문 진행도: ";
    for (Color color : colors) {
        setConsoleColor(color);
        std::cout << colorName(color) << " ";
        for (int door = 0; door < 2; ++door)
            std::cout << doorMarker(door < doorCount(color));
        resetConsoleColor();
        std::cout << "   ";
    }
    std::cout << "\n  내 문: ";
    if (gainedDoors_.empty()) std::cout << "아직 획득한 문이 없습니다.";
    else for (const Card& door : gainedDoors_) {
        setCardColor(door);
        std::cout << "[" << colorName(door.getColor()) << " 문] ";
        resetConsoleColor();
    }
    std::cout << "\n  미로: ";
    if (path_.empty()) std::cout << "아직 걸어간 길이 없습니다.";
    else for (const Card& card : path_) {
        setCardColor(card);
        std::cout << card.shortName();
        resetConsoleColor();
        std::cout << "  ";
    }
    std::cout << "\n  버림 " << discardPile_.size() << "장 | 림보 " << limbo_.size() << "장\n";
    showPiles();
    std::cout << "  -------------------------------------------\n";
}

void GameManager::showPiles() const {
    const std::size_t width = 18;
    const std::vector<std::vector<std::string>> panels = {
        { "덱", "  .----------.  ", "  | ######## |  ", "  '----------'  ", "남은 카드: " + std::to_string(deck_.remaining()) + "장" },
        { "묘지", "  .----------.  ", "  | 버린 카드 |  ", "  '----------'  ", "버린 카드: " + std::to_string(discardPile_.size()) + "장" },
        { "림보", "  .----------.  ", "  | 섞일 카드 |  ", "  '----------'  ", "림보 카드: " + std::to_string(limbo_.size()) + "장" }
    };

    std::cout << "\n  카드 더미\n  ";
    for (int i = 0; i < 3; ++i) std::cout << frameBorder(width) << (i < 2 ? "   " : "\n");
    const std::size_t rows = panels[0].size() + 2;
    for (std::size_t row = 0; row < rows; ++row) {
        std::cout << "  ";
        for (int panel = 0; panel < 3; ++panel) {
            std::string line;
            if (row == 0 || row == rows - 1) line = frameBorder(width);
            else {
                const std::size_t contentRow = row - 1;
                line = framedLine(contentRow < panels[panel].size() ? panels[panel][contentRow] : "", width);
            }
            std::cout << line << (panel < 2 ? "   " : "\n");
        }
    }
}

void GameManager::showHand() const {
    std::cout << "\n  손에 든 미로 카드\n";
    const std::size_t cardsPerRow = 3;
    for (std::size_t start = 0; start < player_.hand().size(); start += cardsPerRow) {
        const std::size_t count = (std::min)(cardsPerRow, player_.hand().size() - start);
        std::vector<std::vector<std::string>> art;
        for (std::size_t i = 0; i < count; ++i)
            art.push_back(player_.hand()[start + i].textArt(static_cast<int>(start + i + 1)));
        for (std::size_t line = 0; line < art[0].size(); ++line) {
            std::cout << "   ";
            for (std::size_t card = 0; card < count; ++card) {
                if (card != 0) std::cout << "   ";
                const Card& currentCard = player_.hand()[start + card];
                setCardColor(currentCard);
                std::cout << art[card][line];
                resetConsoleColor();
            }
            std::cout << "\n";
        }
    }
    if (player_.handCount() == 0) std::cout << "   (손패 없음)\n";
}

void GameManager::dealHand() {
    while (player_.handCount() < 5 && !deck_.empty()) {
        Card card = deck_.draw();
        if (card.getType() == CardType::Path) player_.addCard(card);
        else limbo_.push_back(card); // Setup: doors and nightmares wait in Limbo.
    }
    if (player_.handCount() < 5) gameOver_ = true;
}

void GameManager::playTurn() {
    takeAction();
    if (paused_) return;
    if (!gameOver_ && !victory_) fillHand();
    shuffleLimbo();
    if (!gameOver_ && !victory_) {
        showBoard();
        showHand();
        std::cout << "\n  " << turn_ << "번째 턴이 끝났습니다.\n";
        std::cout << "  잠시 후 자동으로 다음 턴으로 넘어갑니다...\n";
        Sleep(1200);
        clearConsole();
        ++turn_;
    }
    saveGame();
}

void GameManager::takeAction() {
    while (true) {
        showBoard();
        showHand();
        std::cout << "\n  행동: [1] 카드 내기  [2] 카드 버리기  [3] 규칙 보기  [4] 저장 후 타이틀\n";
        int action = readChoice("  선택 > ", 1, 4);
        if (action == 3) {
            std::cout << "\n  규칙 요약\n"
                << "  - 턴마다 손패에서 미로 카드 1장을 내거나 버립니다.\n"
                << "  - 미로에 놓인 직전 카드와 같은 아이콘은 연속으로 낼 수 없습니다.\n"
                << "  - 같은 색 길 카드 3장이 연속되면 그 색 문 1장을 덱에서 얻습니다.\n"
                << "  - 열쇠를 버리면 예언: 덱 위 5장을 보고 1장을 버린 뒤 나머지를 정렬합니다.\n"
                << "  - 손패를 5장까지 채웁니다. 문은 같은 색 열쇠로 얻을 수 있습니다.\n"
                << "  - 악몽은 네 가지 불이익 중 하나를 선택하게 합니다.\n";
            continue;
        }
        if (action == 4) {
            if (saveGame()) std::cout << "\n  저장했습니다. 타이틀 화면으로 돌아갑니다.\n";
            else std::cout << "\n  저장에 실패했습니다.\n";
            paused_ = true;
            return;
        }

        bool returnToActionMenu = false;
        while (!returnToActionMenu) {
            showHand();
            int selected = readChoice("  카드 번호 (0: 행동 메뉴로 돌아가기) > ",
                0, static_cast<int>(player_.hand().size()));
            if (selected == 0) {
                returnToActionMenu = true;
                continue;
            }

            const std::size_t index = static_cast<std::size_t>(selected - 1);
            if (player_.hand()[index].getType() == CardType::Empty) {
                std::cout << "  그 칸은 비어 있습니다. 카드가 있는 번호를 골라주세요.\n";
                continue;
            }
            if (action == 1 && !canPlay(index)) {
                std::cout << "  직전 카드와 같은 아이콘은 이어서 낼 수 없습니다. 다른 카드를 골라주세요.\n";
                continue;
            }

            std::cout << "\n  선택한 카드: " << player_.hand()[index].name() << "\n";
            std::cout << (action == 1
                ? "  [1] 이 카드 내기  [2] 다른 카드 고르기  [3] 행동 메뉴로 돌아가기\n"
                : "  [1] 이 카드 버리기  [2] 다른 카드 고르기  [3] 행동 메뉴로 돌아가기\n");
            int confirmation = readChoice("  선택 > ", 1, 3);
            if (confirmation == 2) continue;
            if (confirmation == 3) {
                returnToActionMenu = true;
                continue;
            }

            if (action == 1) playPath(index);
            else discardCard(index);
            return;
        }
    }
}

bool GameManager::canPlay(std::size_t index) const {
    if (index >= player_.hand().size()) return false;
    const Card& candidate = player_.hand()[index];
    return candidate.getType() == CardType::Path &&
        (path_.empty() || path_.back().getSymbol() != candidate.getSymbol());
}

void GameManager::playPath(std::size_t index) {
    Card card = player_.takeCard(index);
    path_.push_back(card);
    std::cout << "  " << card.name() << " 카드를 미로에 놓았습니다.\n";
    checkDoor();
}

void GameManager::discardCard(std::size_t index) {
    Card card = player_.takeCard(index);
    discardPile_.push_back(card);
    std::cout << "  " << card.name() << " 카드를 버렸습니다.\n";
    if (card.getSymbol() == Symbol::Key) prophecy();
}

void GameManager::prophecy() {
    if (deck_.empty()) {
        std::cout << "  덱에 예언할 카드가 없습니다.\n";
        return;
    }
    std::vector<Card> revealed = deck_.revealTop(5);
    std::cout << "\n  [열쇠 예언] 덱 위 카드 " << revealed.size() << "장을 살펴봅니다.\n";
    for (std::size_t i = 0; i < revealed.size(); ++i)
        std::cout << "   " << (i + 1) << ". " << revealed[i].name() << "\n";
    int discardIndex = readChoice("  버릴 카드 번호 > ", 1, static_cast<int>(revealed.size())) - 1;
    discardPile_.push_back(revealed[static_cast<std::size_t>(discardIndex)]);
    std::vector<Card> remaining;
    for (std::size_t i = 0; i < revealed.size(); ++i)
        if (i != static_cast<std::size_t>(discardIndex)) remaining.push_back(revealed[i]);

    std::vector<Card> topOrder;
    while (!remaining.empty()) {
        std::cout << "  덱 위에 놓을 순서를 정합니다. 남은 카드:\n";
        for (std::size_t i = 0; i < remaining.size(); ++i)
            std::cout << "   " << (i + 1) << ". " << remaining[i].name() << "\n";
        int next = readChoice("  다음으로 맨 위에 놓을 카드 > ", 1, static_cast<int>(remaining.size())) - 1;
        topOrder.push_back(remaining[static_cast<std::size_t>(next)]);
        remaining.erase(remaining.begin() + next);
    }
    deck_.putOnTop(topOrder);
}

void GameManager::fillHand() {
    while (!gameOver_ && !victory_ && player_.handCount() < 5) {
        if (deck_.empty()) {
            gameOver_ = true;
            return;
        }
        drawLocationForHand();
    }
}

void GameManager::drawLocationForHand() {
    Card card = deck_.draw();
    std::cout << "\n  뽑은 카드: " << card.name() << "\n";
    if (card.getType() == CardType::Path) {
        player_.addCard(card);
        return;
    }
    if (card.getType() == CardType::Door) {
        int keyIndex = matchingKeyIndex(card.getColor());
        if (keyIndex >= 0) {
            std::cout << "  같은 색 열쇠가 손에 있습니다. 문을 열까요? [1] 열기  [2] 림보로 보내기\n";
            int answer = readChoice("  선택 > ", 1, 2);
            if (answer == 1) {
                Card key = player_.takeCard(static_cast<std::size_t>(keyIndex));
                discardPile_.push_back(key);
                acquireDoor(card.getColor(), "열쇠 사용");
                return;
            }
        }
        limbo_.push_back(card);
        std::cout << "  문을 얻지 않고 림보에 둡니다.\n";
        return;
    }
    resolveNightmare();
}

void GameManager::resolveNightmare() {
    std::cout << "\n  !!! 악몽 카드가 나타났습니다 !!!\n"
        << "  벌칙을 하나 선택하세요.\n"
        << "  [1] 손패의 열쇠 1장 버리기\n"
        << "  [2] 이미 얻은 문 1장을 림보로 보내기\n"
        << "  [3] 덱 위 최대 5장 공개: 미로 카드는 버리고 문/악몽은 림보로\n"
        << "  [4] 손패 전체를 버리고 새 손패 5장 받기\n";

    bool hasKey = false;
    for (const Card& held : player_.hand()) if (held.getSymbol() == Symbol::Key) hasKey = true;
    int low = 1;
    int high = 4;
    if (!hasKey) std::cout << "  (손패에 열쇠가 없어 1번은 선택할 수 없습니다.)\n";
    if (gainedDoors_.empty()) std::cout << "  (획득한 문이 없어 2번은 선택할 수 없습니다.)\n";
    while (true) {
        int option = readChoice("  선택 > ", low, high);
        if (option == 1 && !hasKey) { std::cout << "  손패에 열쇠가 없습니다.\n"; continue; }
        if (option == 2 && gainedDoors_.empty()) { std::cout << "  림보로 보낼 문이 없습니다.\n"; continue; }

        if (option == 1) {
            std::vector<int> keyIndices;
            std::cout << "  버릴 열쇠를 고르세요:\n";
            for (std::size_t i = 0; i < player_.hand().size(); ++i) {
                if (player_.hand()[i].getSymbol() == Symbol::Key) {
                    keyIndices.push_back(static_cast<int>(i));
                    std::cout << "   " << keyIndices.size() << ". " << player_.hand()[i].name() << "\n";
                }
            }
            int selected = readChoice("  열쇠 번호 > ", 1, static_cast<int>(keyIndices.size())) - 1;
            Card key = player_.takeCard(static_cast<std::size_t>(keyIndices[static_cast<std::size_t>(selected)]));
            discardPile_.push_back(key);
            std::cout << "  " << key.name() << " 열쇠를 버렸습니다.\n";
        } else if (option == 2) {
            std::cout << "  림보로 보낼 문을 고르세요:\n";
            for (std::size_t i = 0; i < gainedDoors_.size(); ++i)
                std::cout << "   " << (i + 1) << ". " << gainedDoors_[i].name() << "\n";
            int selected = readChoice("  문 번호 > ", 1, static_cast<int>(gainedDoors_.size())) - 1;
            limbo_.push_back(gainedDoors_[static_cast<std::size_t>(selected)]);
            gainedDoors_.erase(gainedDoors_.begin() + selected);
            std::cout << "  문을 림보로 보냈습니다.\n";
        } else if (option == 3) {
            std::vector<Card> revealed = deck_.revealTop(5);
            std::cout << "  덱 위 " << revealed.size() << "장을 공개합니다.\n";
            for (const Card& exposed : revealed) {
                std::cout << "   " << exposed.name() << " -> ";
                if (exposed.getType() == CardType::Path) {
                    discardPile_.push_back(exposed);
                    std::cout << "버림\n";
                } else {
                    limbo_.push_back(exposed);
                    std::cout << "림보\n";
                }
            }
        } else {
            for (std::size_t i = 0; i < player_.hand().size(); ++i)
                if (player_.hand()[i].getType() != CardType::Empty)
                    discardPile_.push_back(player_.takeCard(i));
            std::cout << "  손패 전체를 버리고 새 손패를 받습니다.\n";
            dealHand();
        }
        break;
    }
    discardPile_.push_back(Card(CardType::Nightmare));
}

void GameManager::acquireDoor(Color color, const std::string& reason) {
    Card door;
    if (!deck_.takeDoor(color, door)) {
        std::cout << "  " << colorName(color) << " 문이 덱에 없습니다.\n";
        return;
    }
    gainedDoors_.push_back(door);
    std::cout << "  " << colorName(color) << " 문을 획득했습니다! (" << reason << ")\n";
    if (isVictory()) victory_ = true;
}

void GameManager::shuffleLimbo() {
    if (limbo_.empty()) return;
    std::cout << "\n  림보 카드 " << limbo_.size() << "장을 덱에 다시 섞습니다.\n";
    deck_.addAndShuffle(limbo_);
    limbo_.clear();
}

void GameManager::checkDoor() {
    if (path_.size() < 3) return;
    const std::size_t n = path_.size();
    const Color color = path_[n - 1].getColor();
    if (path_[n - 2].getColor() == color && path_[n - 3].getColor() == color && doorCount(color) < 2)
        acquireDoor(color, "같은 색 길 카드 3장 연속");
}

bool GameManager::hasDoor(Color color) const { return doorCount(color) > 0; }

int GameManager::doorCount(Color color) const {
    return static_cast<int>(std::count_if(gainedDoors_.begin(), gainedDoors_.end(),
        [color](const Card& door) { return door.getColor() == color; }));
}

int GameManager::matchingKeyIndex(Color color) const {
    for (std::size_t i = 0; i < player_.hand().size(); ++i)
        if (player_.hand()[i].getType() == CardType::Path &&
            player_.hand()[i].getColor() == color && player_.hand()[i].getSymbol() == Symbol::Key)
            return static_cast<int>(i);
    return -1;
}

bool GameManager::isVictory() const { return gainedDoors_.size() == 8; }

int GameManager::readChoice(const std::string& prompt, int low, int high) const {
    while (true) {
        std::cout << prompt;
        int value;
        if (std::cin >> value && value >= low && value <= high) return value;
        std::cout << "  " << low << "부터 " << high << " 사이의 번호를 입력하세요.\n";
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    }
}

bool GameManager::hasSave() const {
    std::ifstream file("onirim_save.txt");
    return file.good();
}

bool GameManager::saveGame() const {
    std::ofstream file("onirim_save.txt", std::ios::trunc);
    if (!file) return false;
    file << "ONIRIM_SAVE_1\n" << turn_ << ' ' << gameOver_ << ' ' << victory_ << '\n';
    auto writeCards = [&file](const std::vector<Card>& cards) {
        file << cards.size() << '\n';
        for (const Card& card : cards)
            file << static_cast<int>(card.getType()) << ' '
                << static_cast<int>(card.getColor()) << ' '
                << static_cast<int>(card.getSymbol()) << '\n';
    };
    writeCards(deck_.cards());
    writeCards(player_.hand());
    writeCards(path_);
    writeCards(gainedDoors_);
    writeCards(discardPile_);
    writeCards(limbo_);
    return file.good();
}

bool GameManager::loadGame() {
    std::ifstream file("onirim_save.txt");
    std::string signature;
    int loadedTurn = 1;
    bool loadedGameOver = false, loadedVictory = false;
    if (!(file >> signature) || signature != "ONIRIM_SAVE_1" ||
        !(file >> loadedTurn >> loadedGameOver >> loadedVictory)) return false;
    auto readCards = [&file](std::vector<Card>& cards) {
        std::size_t count = 0;
        if (!(file >> count) || count > 1000) return false;
        cards.clear();
        for (std::size_t i = 0; i < count; ++i) {
            int type, color, symbol;
            if (!(file >> type >> color >> symbol) || type < 0 || type > 3 ||
                color < 0 || color > 4 || symbol < 0 || symbol > 3) return false;
            cards.emplace_back(static_cast<CardType>(type), static_cast<Color>(color),
                static_cast<Symbol>(symbol));
        }
        return true;
    };
    std::vector<Card> deckCards, handCards;
    if (!readCards(deckCards) || !readCards(handCards) || handCards.size() != 5 ||
        !readCards(path_) || !readCards(gainedDoors_) || !readCards(discardPile_) ||
        !readCards(limbo_)) return false;
    deck_.setCards(deckCards);
    player_ = Player("꿈꾸는 자");
    player_.setHand(handCards);
    turn_ = loadedTurn;
    gameOver_ = loadedGameOver;
    victory_ = loadedVictory;
    paused_ = false;
    return true;
}

void GameManager::run() {
    while (true) {
        clearConsole();
        showTitle();
        std::cout << "  [1] 새 게임\n"
            << "  [2] 이어하기";
        if (!hasSave()) std::cout << " (저장 없음)";
        std::cout << "\n"
            << "  [3] 튜토리얼\n"
            << "  [4] 규칙\n"
            << "  [5] 종료\n";
        const int menu = readChoice("  메뉴 선택 > ", 1, 5);
        if (menu == 5) return;
        if (menu == 3) {
            showTutorial();
            continue;
        }
        if (menu == 4) {
            showRules();
            waitForEnter("\n  Enter를 누르면 타이틀로 돌아갑니다... ");
            continue;
        }
        if (menu == 2) {
            if (!loadGame()) {
                std::cout << "\n  이어할 저장 데이터가 없거나 손상되었습니다.\n";
                waitForEnter("  Enter를 누르세요... ");
                continue;
            }
        } else {
            resetGame();
            dealHand();
            shuffleLimbo();
            saveGame();
        }

        clearConsole();
        while (!gameOver_ && !victory_ && !paused_) playTurn();
        if (paused_) continue;
        showBoard();
        std::cout << "\n  +----------------------------------------------------------+\n";
        if (victory_) std::cout << "  |             승리! 여덟 개의 문을 모두 찾았습니다.         |\n";
        else std::cout << "  |          패배... 손패를 채울 덱이 바닥났습니다.           |\n";
        std::cout << "  +----------------------------------------------------------+\n\n";
        waitForEnter("  Enter를 누르면 타이틀 화면으로 돌아갑니다... ");
    }
}

