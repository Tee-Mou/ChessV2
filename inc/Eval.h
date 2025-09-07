#include "MoveGen.h"

#define TRANSPOSITION_CACHE_SIZE 0xFFFFFFF
#define INVALID_TRANSPOSITION_EVAL 10101010

struct KillerMoves
{
    public:
        void addNewKiller(uint move) {
            first = second;
            second = move;
        };
        uint first = 0;
        uint second = 0;
        uint mPly;
};
enum NodeType {ALPHA, BETA, EXACT};
struct Transposition {
    void init(u64 addKey, MoveGen::MoveData* addRefutation, uint addDepth, float addEval, NodeType addType) {
        key = addKey; refutation = addRefutation; depth = addDepth; eval = addEval; type = addType;
    };
    u64 key;
    MoveGen::MoveData* refutation;
    uint depth;
    float eval;
    NodeType type;
};

class Eval 
{
    public:
        static constexpr int PIECEVALUES[7] = {0, 0, 1, 3, 3, 5, 9};
        uint calculateMoveOrderScore(Board* board, MoveGen::MoveData moveData);
        static bool compareByScore(MoveGen::MoveData &a, MoveGen::MoveData &b) {
            return a.score < b.score;
        };
        static float evaluatePosition(Board* board);
        float evalAlphaBeta(Board* board, uint depth, float alpha, float beta);

        void addTransposition(Transposition tp);
        float checkTransposition(u64 hashKey, uint depth, float alpha, float beta);

        uint currentDepth;
        Transposition transpositionCache[TRANSPOSITION_CACHE_SIZE];
        std::vector<KillerMoves> killers;
};
