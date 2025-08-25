#include "../inc/Game.h"
#include "../inc/BitOps.h"

unsigned long long Game::findPseudoLegalMoves() {
    // std::cout << "finding moves" << std::endl;
    unsigned long long pseudoLegalMoves = 0;
    // find pawn moves
    unsigned long long pawnMoves;
    if (currentTurn) {
        unsigned long long originalRank = pawnMasks[0];
        unsigned long long pawns = pieceLocations[2];
        unsigned long long pawnPushes = ((pawns & originalRank) << 8)  & ~pieceLocations[0];
        pawnPushes |= (((pawnPushes << 8) | (pieceLocations[9] & ~originalRank) << 8) & ~pieceLocations[0]);
        unsigned long long captures;
        unsigned long long capturesLeft = ((pieceLocations[2] & edgeMasks[2]) << 7) & pieceLocations[8];
        unsigned long long capturesRight = ((pieceLocations[2] & edgeMasks[3]) << 9) & pieceLocations[8];
        captures = capturesLeft | capturesRight;
        pawnMoves = pawnPushes | captures;
    } else {
        unsigned long long originalRank = pawnMasks[1];
        unsigned long long pawns = pieceLocations[9];
        unsigned long long pawnPushes = (pieceLocations[9] >> 8 & originalRank & ~pieceLocations[0]);
        pawnPushes |= (((pawnPushes >> 8) | (pieceLocations[9] & ~originalRank) >> 8) & ~pieceLocations[0]);
        unsigned long long captures;
        unsigned long long capturesLeft = ((pieceLocations[9] & edgeMasks[2]) >> 9) & pieceLocations[1];
        unsigned long long capturesRight = ((pieceLocations[9] & edgeMasks[3]) >> 7) & pieceLocations[1];
        captures = capturesLeft | capturesRight;
        pawnMoves = pawnPushes | captures;
    };
    // implement en passant

    // find king moves
    unsigned long long kingMoves = 0;
    int square = BitOps::countTrailingZeroes(pieceLocations[currentTurn ? 3 : 10]);
    kingMoves |= kingMovesTable[square];
    kingMoves &= ~pieceLocations[currentTurn ? 1 : 8];
    // implement checking

    // find bishop moves 
    unsigned long long bishopLocations = pieceLocations[currentTurn ? 5 : 12];
    unsigned long long bishopMoves = findBishopMoves(bishopLocations);

    // find knight moves
    unsigned long long knightMoves = 0;
    square = 0;
    unsigned long long knightLocations = pieceLocations[currentTurn ? 6 : 13];
    while (knightLocations != 0) {
        square += BitOps::countTrailingZeroes(knightLocations);
        knightMoves |= knightMovesTable[square];
        knightLocations >>= (square + 1);
        square++;
    };

    //find rook moves
    unsigned long long rookLocations = pieceLocations[currentTurn ? 7 : 14];
    unsigned long long rookMoves = findRookMoves(rookLocations);

    
    // find queen moves
    unsigned long long queenLocations = pieceLocations[currentTurn ? 4 : 12];
    unsigned long long queenMoves = 0;
    queenMoves |= findBishopMoves(queenLocations);
    queenMoves |= findRookMoves(queenLocations);

    pseudoLegalMoves |= pawnMoves | kingMoves | queenMoves | bishopMoves | knightMoves | rookMoves;
    pseudoLegalMoves &= ~pieceLocations[currentTurn ? 1 : 8];
    return pseudoLegalMoves;
}

unsigned long long Game::findBishopMoves(unsigned long long bitboard) {
    unsigned long long bishopMoves = 0;
    int square = 0;
    while (bitboard != 0) {
        square += BitOps::countTrailingZeroes(bitboard);
        unsigned long long mask = diagonalMasks[square];
        unsigned long long relevantSquares = pieceLocations[0] & mask;
        unsigned long long magicNumber = bishopMagics[square];
        int shiftIndex = 64 - relevantBitsBishop[square];
        bishopMoves |= bishopAttacks[square][(relevantSquares * magicNumber) >> shiftIndex];
        bitboard >>= (square + 1);
        square++;
    }
    return bishopMoves;
}

unsigned long long Game::findRookMoves(unsigned long long bitboard) {
    unsigned long long rookMoves = 0;
    int square = 0;
    while (bitboard != 0) {
        square += BitOps::countTrailingZeroes(bitboard);
        unsigned long long mask = cardinalMasks[square];
        unsigned long long relevantSquares = pieceLocations[0] & mask;
        unsigned long long magicNumber = rookMagics[square];
        int shiftIndex = 64 - relevantBitsRook[square];
        rookMoves |= rookAttacks[square][(relevantSquares * magicNumber) >> shiftIndex];
        bitboard >>= (square + 1);
        square++;
    }
    return rookMoves;
}

// 00000001 00000000 00000000
// 00000001 00000000 10000001
// 00000001 10000001 10000001
// 10000001 01000010 10000001
// 11000001 00100100 10000001
// 10100111 10011001 10000001
// 11111101 01011010 10000001
// 00000000 00x00x00 x100001x