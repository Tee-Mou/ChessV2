#include <iostream>
#include "inc/Eval.h"

void main() {
    Board* board = new Board();
    Eval* evaluator = new Eval(board);
    float eval = evaluator->evalAlphaBeta(5, -INFINITY, INFINITY);
    std::cout << board->pieceLocations[0] << std::endl;
    std::cout << eval << std::endl;
    getchar();
}