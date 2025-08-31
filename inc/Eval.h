#include "MoveGen.h"

class Eval 
{
    public: 
        static uint calculateMoveScore(uint moveData) {
            uint score;
            return 1;
        };
        static bool compareByScore(MoveGen::Move &a, MoveGen::Move &b) {
            return a.moveScore < b.moveScore;
        }
};