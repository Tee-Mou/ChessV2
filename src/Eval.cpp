#include "../inc/Eval.h"
#include "../inc/BitOps.h"

uint Eval::calculateMoveOrderScore(uint move) {
    // Assess whether captures are possible
    // Favour castling early
    return 1;
}

float Eval::evaluatePosition(Board* board) {
    const int PIECEVALUES[7] = {0, 0, 100, 300, 300, 500, 900};
    float positionScore = 0;
    for (int i = 3; i < 15; i++) {
        // Assess relative piece value
        positionScore += BitOps::countSetBits(board->pieceLocations[i]) * PIECEVALUES[((i-1)  % 7)] * (i < 8 ? 1 : -1);
        // Assess strength of each piece's square
        // Assess whether favourable captures exist (attack/defend bitboards)
    }
    return positionScore;
}

float Eval::evalAlphaBeta(Board* board, std::vector<MoveGen::MoveData> moves, uint ply, float alpha, float beta) {
    float eval;
    if (ply = 0) { return evaluatePosition(board); }
    if (board->currentTurn) {
        eval = -INFINITY;
        std::vector<MoveGen::MoveData>::iterator it;
        for (it = moves.begin(); it != moves.end(); ++it) {
            std::vector<MoveGen::MoveData> nextMoves = MoveGen::findLegalMoves(board, MoveGen::findPseudoLegalMoves(board));
            std::vector<MoveGen::MoveData>::iterator childIt;
            for(childIt = nextMoves.begin(); childIt != nextMoves.end(); ++childIt) { 
                MoveGen::MoveData move = *childIt;
                Board* childBoard = MoveGen::doMove(board, move.move);
                eval = std::max(eval, evalAlphaBeta(childBoard, nextMoves, ply-1, alpha, beta));
                if (eval >= beta) {
                    break;
                alpha = std::max(alpha, eval);
                }
            return eval;
            }
        }
    }
    else {
        eval = INFINITY;
        std::vector<MoveGen::MoveData>::iterator it;
        for (it = moves.begin(); it != moves.end(); ++it) {
            std::vector<MoveGen::MoveData> nextMoves = MoveGen::findLegalMoves(board, MoveGen::findPseudoLegalMoves(board));
            std::vector<MoveGen::MoveData>::iterator childIt;
            for(childIt = nextMoves.begin(); childIt != nextMoves.end(); ++childIt) { 
                MoveGen::MoveData move = *childIt;
                Board* childBoard = MoveGen::doMove(board, move.move);
                eval = std::min(eval, evalAlphaBeta(childBoard, nextMoves, ply-1, alpha, beta));
                if (eval <= alpha) {
                    break;
                beta = std::min(beta, eval);
                }
            return eval;
            }
        }
    }
};