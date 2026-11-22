#include "Game.h"

#include <iostream>
#include <windows.h>

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    GameManager game;
    game.run();
    std::cout << "엔터 키를 누르면 종료합니다...";
    std::cin.ignore();
    std::cin.get();
    return 0;
}
