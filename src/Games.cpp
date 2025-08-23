#include "../inc/Game.h"

long long Game::generatePseudoLegalMoves() {
    long long pseudoLegalMoves = 0;
    // find pawn moves
    long long pawnMoves;
    if (currentTurn) {
        long long originalRank = pawnMasks[0];
        long long pawns = pieceLocations[2];
        long long pawnPushes = (pieceLocations[2] << 8 & originalRank & !pieceLocations[0]);
        pawnPushes |= (((pawnPushes << 8) | (pieceLocations[9] & !originalRank) << 8) & !pieceLocations[0]);
        long long captures;
        long long capturesLeft = ((pieceLocations[2] & edgeMasks[2]) << 7) & pieceLocations[8];
        long long capturesRight = ((pieceLocations[2] & edgeMasks[3]) << 9) & pieceLocations[8];
        captures = capturesLeft | capturesRight;
        pawnMoves = pawnPushes | captures;
    } else {
        long long originalRank = pawnMasks[1];
        long long pawns = pieceLocations[9];
        long long pawnPushes = (pieceLocations[9] >> 8 & originalRank & !pieceLocations[0]);
        pawnPushes |= (((pawnPushes >> 8) | (pieceLocations[9] & !originalRank) >> 8) & !pieceLocations[0]);
        long long captures;
        long long capturesLeft = ((pieceLocations[9] & edgeMasks[2]) >> 9) & pieceLocations[1];
        long long capturesRight = ((pieceLocations[9] & edgeMasks[3]) >> 7) & pieceLocations[1];
        captures = capturesLeft | capturesRight;
        pawnMoves = pawnPushes | captures;
    };

    // find king moves
    long long kingMoves;
    if (currentTurn) {
        kingMoves = (
            ((pieceLocations[3] & edgeMasks[0]) << 8)
            |((pieceLocations[3] & edgeMasks[0] & edgeMasks[2]) << 7)
            |((pieceLocations[3] & edgeMasks[0] & edgeMasks[3]) << 9)
            |((pieceLocations[3] & edgeMasks[2]) >> 1)
            |((pieceLocations[3] & edgeMasks[3]) << 1)
            |((pieceLocations[3] & edgeMasks[1]) >> 8)
            |((pieceLocations[3] & edgeMasks[1] & edgeMasks[2]) >> 9)
            |((pieceLocations[3] & edgeMasks[1] & edgeMasks[3]) >> 7)
        ) & !pieceLocations[currentTurn];
    }

    return pseudoLegalMoves;
}