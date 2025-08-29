#include <stdexcept>
#include "../inc/Game.h"
#include "../inc/BitOps.h"

u64 Game::findPseudoLegalMoves() {
    u64 pseudoLegalMoves = 0;

    u64 pawnLocations = pieceLocations[currentTurn ? 3 : 10];
    u64 pawnMoves = findPawnMoves(pawnLocations);

    int kingSquare = BitOps::countTrailingZeroes(pieceLocations[currentTurn ? 2 : 9]);
    u64 kingMoves = findKingMoves(kingSquare);
    // implement castling

    u64 bishopLocations = pieceLocations[currentTurn ? 5 : 12];
    u64 bishopMoves = findBishopMoves(bishopLocations);

    u64 knightLocations = pieceLocations[currentTurn ? 6 : 13];
    u64 knightMoves = findKnightMoves(knightLocations);

    u64 rookLocations = pieceLocations[currentTurn ? 7 : 14];
    u64 rookMoves = findRookMoves(rookLocations);

    u64 queenLocations = pieceLocations[currentTurn ? 4 : 12];
    u64 queenMoves = 0;
    queenMoves |= findBishopMoves(queenLocations);
    queenMoves |= findRookMoves(queenLocations);

    pseudoLegalMoves |= pawnMoves | kingMoves | queenMoves | bishopMoves | knightMoves | rookMoves;
    pseudoLegalMoves &= ~pieceLocations[currentTurn ? 1 : 8];
    return pseudoLegalMoves;
}

void Game::doMove(unsigned int move) {
    unsigned int oldSquare = (move & 0b00000000000111111);
    unsigned int newSquare = (move & 0b00000111111000000) >> 6;
    unsigned int captured = (move & 0b00111000000000000) >> 12; // 'capture' order (ascending value): P -> Q -> B -> N -> R -> O-O -> O-O-O
    unsigned int promoteTo = (move & 0b11000000000000000) >> 15; // 'promotion' order (ascending value): Q -> B -> N -> R

    int relevantBitboard = 16;
    for (int i = currentTurn ? 2 : 9; currentTurn ? i < 8 : i < 15; i++) {
        if ((1ULL << oldSquare) & pieceLocations[i]) {
            relevantBitboard = i;
            break;
        };
    };
    if (0 < captured < 6) { // Handle capturing
        unsigned int opponentBitboard = captured + 1 + (!currentTurn * 7);
        pieceLocations[opponentBitboard] &= ~(1ULL << newSquare); // Remove from opponent's piece's board
        pieceLocations[currentTurn ? 8 : 1] &= ~(1ULL << newSquare); // Remove from opponent's board
    }
    else if (6 <= captured <= 7) { // Handle castling
        bool longCastle = captured - 6;
        unsigned int rookBitboard = currentTurn ? 7 : 14;
        unsigned int oldRookSquare = 1ULL << ((longCastle ? 0 : 7) + (currentTurn ? 0 : 56));
        unsigned int newRookSquare = 1ULL << ((longCastle ? 3 : 5) + (!currentTurn ? 0 : 56));
        pieceLocations[rookBitboard] &= ~oldRookSquare; // Update rook bitboard
        pieceLocations[rookBitboard] |= newRookSquare;
    };
    if (promoteTo != 0) { // Handle promotion
        unsigned int promotedToBitboard = promoteTo + 3 + (!currentTurn * 7); 
        pieceLocations[promotedToBitboard] |= (1ULL << newSquare); // Add to new piece's bitboard
        relevantBitboard = 16; // Don't update pawn bitboard with new location
    }

    // Add to bitboard for piece type
    try {
        pieceLocations[relevantBitboard] & ~(1ULL << oldSquare);
        pieceLocations[relevantBitboard] |= (1ULL << newSquare);
    } catch (const std::out_of_range &e) {};

    // Add to bitboard for all pieces 
    pieceLocations[0] & ~(1ULL << oldSquare);
    pieceLocations[0] |= (1ULL << newSquare);
    
    // Add to bitboard for piece's colour
    pieceLocations[currentTurn ? 1 : 8] & ~(1ULL << oldSquare);
    pieceLocations[currentTurn ? 1 : 8] |= (1ULL << newSquare);
}

void Game::undoMove(unsigned int move) {
    unsigned int oldSquare = (move & 0b00000000000111111);
    unsigned int newSquare = (move & 0b00000111111000000) >> 6;
    unsigned int captured = (move & 0b00111000000000000) >> 12; // 'capture' order (ascending value): P -> Q -> B -> N -> R -> O-O -> O-O-O
    unsigned int promoteTo = (move & 0b11000000000000000) >> 15; // 'promotion' order (ascending value): Q -> B -> N -> R

    int relevantBitboard = 16;
    for (int i = currentTurn ? 2 : 9; currentTurn ? i < 8 : i < 15; i++) {
        if ((1ULL << newSquare) & pieceLocations[i]) {
            relevantBitboard = i;
            break;
        };
    };
    if (0 < captured < 6) { // Handle capturing
        unsigned int opponentBitboard = captured + 1 + (!currentTurn * 7);
        pieceLocations[opponentBitboard] |= (1ULL << newSquare); // Remove from opponent's piece's board
        pieceLocations[currentTurn ? 8 : 1] |= (1ULL << newSquare); // Add to opponent's board
    }
    else if (6 <= captured <= 7) { // Handle castling
        bool longCastle = captured - 6;
        unsigned int rookBitboard = currentTurn ? 7 : 14;
        unsigned int oldRookSquare = 1ULL << ((longCastle ? 0 : 7) + (currentTurn ? 0 : 56));
        unsigned int newRookSquare = 1ULL << ((longCastle ? 3 : 5) + (!currentTurn ? 0 : 56));
        pieceLocations[rookBitboard] &= ~newRookSquare; // Update rook bitboard
        pieceLocations[rookBitboard] |= oldRookSquare;
    };
    if (promoteTo != 0) { // Handle promotion
        unsigned int promotedToBitboard = promoteTo + 3 + (!currentTurn * 7); 
        pieceLocations[promotedToBitboard] &= ~(1ULL << newSquare); // Remove from new piece's bitboard
        relevantBitboard = currentTurn ? 3 : 10;
    }

    // Remove from bitboard for piece type
    try {
        pieceLocations[relevantBitboard] & ~(1ULL << newSquare);
        pieceLocations[relevantBitboard] |= (1ULL << oldSquare);
    } catch (const std::out_of_range &e) {};

    // Add to bitboard for all pieces
    if (!captured) { pieceLocations[0] & ~(1ULL << newSquare); }
    pieceLocations[0] |= (1ULL << newSquare);
    
    // Add to bitboard for piece's colour
    pieceLocations[currentTurn ? 1 : 8] & ~(1ULL << oldSquare);
    pieceLocations[currentTurn ? 1 : 8] |= (1ULL << newSquare);
}

u64 Game::findKingMoves(int square) {
    u64 kingMoves = 0;
    kingMoves |= kingMovesTable[square];
    Castles castles = currentTurn ? blackCastles : whiteCastles;
    u64 longCastleMask = 0b00001110;
    u64 shortCastleMask = 0b01100000;
    if (!currentTurn) { longCastleMask << 56; shortCastleMask << 56; };
    if (castles.longCastle & !(longCastleMask & pieceLocations[0])) {
        kingMoves |= 1ULL << (currentTurn ? 2: 58);
    };
    if (castles.shortCastle & !(shortCastleMask & pieceLocations[0])) {        
        kingMoves |= 1ULL << (currentTurn ? 6: 62);
    };
    return kingMoves;
}

u64 Game::findPawnMoves(u64 bitboard) {
    u64 pawnMoves;
    u64 originalRank = currentTurn ? pawnMasks[0] : pawnMasks[1];
    u64 opponentPawns = currentTurn ? pieceLocations[10] : pieceLocations[3];
    u64 pawnPushes = (currentTurn ? ((bitboard & originalRank) << 8) : ((bitboard & originalRank) >> 8)) & ~pieceLocations[0];
    pawnPushes |= (((currentTurn ? pawnPushes << 8 : pawnPushes >> 8) | (bitboard) << 8) & ~pieceLocations[0]);
    
    u64 captures = (currentTurn ? (bitboard & edgeMasks[2]) << 7 : (bitboard & edgeMasks[2]) >> 9) & opponentPawns;
    captures |= (currentTurn ? (bitboard & edgeMasks[3]) << 9 : (bitboard & edgeMasks[3]) >> 7) & opponentPawns;

    u64 enPassantCaptures;
    u64 pawnsCanEnPassant = bitboard & pawnMasks[2];
    int square;
    while (pawnsCanEnPassant != 0) {
        square += BitOps::countTrailingZeroes(pawnsCanEnPassant);
        u64 location = 1ULL << square;
        u64 enPassantBoard = currentTurn ? pawnMasks[2] : pawnMasks[3];
        enPassantCaptures |= ((currentTurn ? location << 7 : location >> 9) & enPassantBoard) | ((currentTurn ? location << 9 : location >> 7) & enPassantBoard);
        pawnsCanEnPassant >>= (square + 1);
        square++;
    };
    pawnMoves = pawnPushes | captures | enPassantCaptures;
    return pawnMoves;
}

u64 Game::findBishopMoves(u64 bitboard) {
    u64 bishopMoves = 0;
    int square = 0;
    while (bitboard != 0) {
        square += BitOps::countTrailingZeroes(bitboard);
        u64 mask = diagonalMasks[square];
        u64 relevantSquares = pieceLocations[0] & mask;
        u64 magicNumber = bishopMagics[square];
        int shiftIndex = 64 - relevantBitsBishop[square];
        bishopMoves |= bishopAttacks[square][(relevantSquares * magicNumber) >> shiftIndex];
        bitboard >>= (square + 1);
        square++;
    };
    return bishopMoves;
}

u64 Game::findKnightMoves(u64 bitboard) {
    u64 knightMoves = 0;
    int square = 0;
    while (bitboard != 0) {
        square += BitOps::countTrailingZeroes(bitboard);
        knightMoves |= knightMovesTable[square];
        bitboard >>= (square + 1);
        square++;
    };
    return knightMoves;
}

u64 Game::findRookMoves(u64 bitboard) {
    u64 rookMoves = 0;
    int square = 0;
    while (bitboard != 0) {
        square += BitOps::countTrailingZeroes(bitboard);
        u64 mask = cardinalMasks[square];
        u64 relevantSquares = pieceLocations[0] & mask;
        u64 magicNumber = rookMagics[square];
        int shiftIndex = 64 - relevantBitsRook[square];
        rookMoves |= rookAttacks[square][(relevantSquares * magicNumber) >> shiftIndex];
        bitboard >>= (square + 1);
        square++;
    };
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