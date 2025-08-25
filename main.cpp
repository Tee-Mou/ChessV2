#include <iostream>
#include "inc/Game.h"
#include "inc/BitOps.h"

void main() {
    Game* game = new Game();
    long long moves = game -> findPseudoLegalMoves();
    std::cout << "Done!" << std::endl;
    getchar();
}