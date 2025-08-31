#include <iostream>
#include "inc/MoveGen.h"
#include "inc/BitOps.h"

void main() {
    MoveGen* mg = new MoveGen();
    std::vector<uint> moves = mg -> findPseudoLegalMoves();
    std::cout << "Done!" << std::endl;
    getchar();
}