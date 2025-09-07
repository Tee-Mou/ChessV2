#include "../inc/Eval.h"
#include "../inc/BitOps.h"

#define DEFAULT_VALUE_MODIFIER 1
#define PAWN_CHAIN_VALUE 0.1
#define PAWN_STACK_VALUE -0.1

uint Eval::calculateMoveOrderScore(Board* board, MoveGen::MoveData moveData) {
    float score = 0;
    // 1 refutation from TT (handled elsewhere)
    // 2 killer moves
    if (moveData.move == killers[currentDepth].first) { return INFINITY - 1; }
    else if (moveData.move == killers[currentDepth].second) { return INFINITY - 2; };
    // 3 captures in order of risk/reward
    uint capture = (moveData.move & 0b00111000000000000) >> 12;
    if (capture) {
        if (capture >= 6) {
            score += capture;
        }
        else {            
            score += PIECEVALUES[capture+1] * 2;
        }
    };
    // 4 checks
    if (moveData.check) { score += 1; };
    // 5 promotions
    uint promotion = (moveData.move & 0b11000000000000000) >> 15;
    if (promotion) {
        score += PIECEVALUES[promotion + 2];
    }
    return score;
};

void Eval::addTransposition(Transposition tp) {
    //TODO: add replacement logic

    transpositionCache[tp.key % TRANSPOSITION_CACHE_SIZE] = tp;
};

float Eval::checkTransposition(u64 hashKey, uint depth, float alpha, float beta) {
    Transposition tp = transpositionCache[hashKey % TRANSPOSITION_CACHE_SIZE];
    if (tp.key == hashKey && tp.depth >= depth) {
        if (tp.type == EXACT) { return tp.eval; };
        if (tp.type == ALPHA && tp.eval <= alpha)  { return alpha; };
        if (tp.type == BETA && tp.eval >= beta)  { return beta; };
    };
    return INVALID_TRANSPOSITION_EVAL;
};

float Eval::evaluatePosition(Board* board) {
    const int PIECEVALUES[7] = {0, 0, 100, 300, 300, 500, 900};
    float score = 0;
    for (int i = 3; i < 15; i++) {
        // Assess relative piece value
        if (i == 8 || i == 9) { continue; }
        u64 bitboard = board->pieceLocations[i];
        while (bitboard > 0) {
            uint square = BitOps::countTrailingZeroes(bitboard);
            float multiplier = DEFAULT_VALUE_MODIFIER;
            if (i == 5 || i == 12) { HeatMaps::knight[square]; };
            float pieceValue = PIECEVALUES[((i-1)  % 7)] * (i < 8 ? 1 : -1);
            score += pieceValue * multiplier;
            
            bitboard ^= 1ULL << square;
        }
    }
    // Assess pawn structure
    u64 whitePawnChains = (board->pieceLocations[3] << 9 || board->pieceLocations[3] << 7) && board->pieceLocations[3];
    u64 blackPawnChains = (board->pieceLocations[10] >> 9 || board->pieceLocations[10] >> 7) && board->pieceLocations[10];
    score += PAWN_CHAIN_VALUE * BitOps::countSetBits(whitePawnChains);
    score -= PAWN_CHAIN_VALUE * BitOps::countSetBits(blackPawnChains);
    u64 whitePawnStacks;
    u64 blackPawnStacks;
    for (uint rank = 1; rank < 8; ++rank) {
        whitePawnStacks |= (board->pieceLocations[3] << rank * 8) & board->pieceLocations[3];
        blackPawnStacks |= (board->pieceLocations[10] >> rank * 8) & board->pieceLocations[10];
    }
    score += PAWN_STACK_VALUE * BitOps::countSetBits(blackPawnChains);
    score -= PAWN_STACK_VALUE * BitOps::countSetBits(whitePawnStacks);

    // Assess whether favourable captures exist (attack/defend bitboards)


    return score;
};

float Eval::evalAlphaBeta(Board* board, uint depth, float alpha, float beta) {
    this->currentDepth++;
    if (depth == 0) { currentDepth = 0; return evaluatePosition(board); };
    std::vector<MoveGen::MoveData> moves = MoveGen::findLegalMoves(board, MoveGen::findPseudoLegalMoves(board));
    if (sizeof(moves) == 0) {
        return INFINITY * board->currentTurn ? 1 : -1;
    };
    if (int eval = checkTransposition(board->zobristHash, alpha, beta, depth) != INVALID_TRANSPOSITION_EVAL) {
        return eval;
    }
    if (board->currentTurn) {
        float eval = -INFINITY;
        std::vector<MoveGen::MoveData>::iterator it;
        MoveGen::MoveData storeMove;
        NodeType tpNodeType;
        for (it = moves.begin(); it != moves.end(); ++it) {
            MoveGen::MoveData move = *it;
            MoveGen::doMove(board, move.move);
            eval = std::max(eval, evalAlphaBeta(board, depth - 1, alpha, beta));
            if (eval >= beta) {
                killers[currentDepth].addNewKiller(move.move);
                storeMove = move;
                tpNodeType = BETA;
                MoveGen::undoMove(board, move.move);
                break;
            }
            if (eval > alpha) {
                alpha = eval;
                storeMove = move;
                tpNodeType = EXACT;
            }
            MoveGen::undoMove(board, move.move);
        }
        Transposition tp;
        tp.init(board->zobristHash, &storeMove, depth, eval, tpNodeType);
        addTransposition(tp);
        return eval;
    }
    else {
        float eval = INFINITY;
        std::vector<MoveGen::MoveData>::iterator it;
        MoveGen::MoveData storeMove;
        NodeType tpNodeType;
        for (it = moves.begin(); it != moves.end(); ++it) {
            MoveGen::MoveData move = *it;
            MoveGen::doMove(board, move.move);
            eval = std::max(eval, evalAlphaBeta(board, depth - 1, alpha, beta));
            if (eval <= alpha) {
                killers[currentDepth].addNewKiller(move.move);
                storeMove = move;
                tpNodeType = ALPHA;
                MoveGen::undoMove(board, move.move);
                break;
            }
            if (eval < beta) {
                beta = eval;
                storeMove = move;
                tpNodeType = EXACT;
            }
            MoveGen::undoMove(board, move.move);
        }
        Transposition tp;
        tp.init(board->zobristHash, &storeMove, depth, eval, tpNodeType);
        addTransposition(tp);
        return eval;
    }
        
};