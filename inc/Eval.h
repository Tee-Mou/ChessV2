#include "MoveGen.h"

class Eval 
{
    public:
        Eval();
        static uint calculateMoveOrderScore(uint moveData);
        static bool compareByScore(MoveGen::MoveData &a, MoveGen::MoveData &b) {
            return a.score < b.score;
        };
        static float evaluatePosition(Board* board);
        static float evalAlphaBeta(Board* board, std::vector<MoveGen::MoveData> move, uint ply, float alpha, float beta);

};