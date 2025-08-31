
#include "../inc/MoveGen.h"
#include "../inc/Eval.h"
#include "../inc/BitOps.h"
#include <algorithm>

std::vector<uint> MoveGen::findPseudoLegalMoves() {
    std::vector<uint> moves;
    std::vector<uint> pseudoLegalMoves;

    u64 pawnLocations = pieceLocations[currentTurn ? 3 : 10];
    std::vector<uint> pawnMoves = findPawnMoves(pawnLocations);

    uint kingSquare = BitOps::countTrailingZeroes(pieceLocations[currentTurn ? 2 : 9]);
    std::vector<uint> kingMoves = findKingMoves(kingSquare);
    // implement castling

    u64 bishopLocations = pieceLocations[currentTurn ? 5 : 12];
    std::vector<uint> bishopMoves = findBishopMoves(bishopLocations);

    u64 knightLocations = pieceLocations[currentTurn ? 6 : 13];
    std::vector<uint> knightMoves = findKnightMoves(knightLocations);

    u64 rookLocations = pieceLocations[currentTurn ? 7 : 14];
    std::vector<uint> rookMoves = findRookMoves(rookLocations);

    u64 queenLocations = pieceLocations[currentTurn ? 4 : 12];
    std::vector<uint> queenMoves;
    std::vector<uint> queenDiagonalMoves = findBishopMoves(queenLocations);
    std::vector<uint> queenCardinalMoves = findRookMoves(queenLocations);
    queenMoves.insert(queenMoves.end(), queenDiagonalMoves.begin(), queenDiagonalMoves.end());
    queenMoves.insert(queenMoves.end(), queenCardinalMoves.begin(), queenCardinalMoves.end());
    
    moves.insert(moves.end(), pawnMoves.begin(), pawnMoves.end());
    std::vector<uint>::iterator it;
    for (it = moves.begin(); it != moves.end(); ++it) {
        uint move = *it;
        uint oldSquare = move & 0b000000111111;
        uint newSquare = (move & 0b111111000000) >> 6;
        // Prevent capturing own pieces
        if ((1ULL << newSquare) & pieceLocations[currentTurn ? 1 : 8]) { continue; }
        // Handle capturing
        if ((1ULL << newSquare) & pieceLocations[currentTurn ? 8 : 1]) {
            uint captureValue = 1;
            if ((1ULL << newSquare) & pieceLocations[(captureValue + 2) + currentTurn ? 8 : 1]) {
                move += (captureValue << 12);
            }
        }
        // Handle promotion
        if (oldSquare & pieceLocations[currentTurn ? 3 : 10] && newSquare >= 56) {
            // Iterate over each possible promotion
            uint promotionValue = 1;
            move += (promotionValue << 15);
            pseudoLegalMoves.push_back(move);
            continue;
        }
        pseudoLegalMoves.push_back(move);
    }
    return pseudoLegalMoves;
    
}

std::vector<MoveGen::Move> MoveGen::findLegalMoves(std::vector<uint> moveList) {
    std::vector<MoveGen::Move> legalMoves;
    std::vector<uint>::iterator it;
    for(it = moveList.begin(); it != moveList.end(); ++it) {
        uint move = *it;
        doMove(move);
        if (checksAreValid(move)) {
            Move moveData;
            moveData.data = move;
            moveData.moveScore = Eval::calculateMoveScore(move);
            legalMoves.push_back(moveData);
        }
        undoMove(move);
    }
    std::sort(legalMoves.begin(), legalMoves.end(), Eval::compareByScore);
    return legalMoves;
}

void MoveGen::doMove(uint move) {
    uint oldSquare = (move & 0b00000000000111111);
    uint newSquare = (move & 0b00000111111000000) >> 6;
    uint captured = (move & 0b00111000000000000) >> 12; // 'capture' order (ascending value): P -> Q -> B -> N -> R -> O-O -> O-O-O
    uint promoteTo = (move & 0b11000000000000000) >> 15; // 'promotion' order (ascending value): Q -> B -> N -> R

    uint relevantBitboard = 16;
    for (uint i = currentTurn ? 2 : 9; currentTurn ? i < 8 : i < 15; i++) {
        if ((1ULL << oldSquare) & pieceLocations[i]) {
            relevantBitboard = i;
            break;
        };
    };
    if (0 < captured < 6) { // Handle capturing
        uint opponentBitboard = captured + 1 + (!currentTurn * 7);
        pieceLocations[opponentBitboard] &= ~(1ULL << newSquare); // Remove from opponent's piece's board
        pieceLocations[currentTurn ? 8 : 1] &= ~(1ULL << newSquare); // Remove from opponent's board
    }
    else if (6 <= captured <= 7) { // Handle castling
        bool longCastle = captured - 6;
        uint rookBitboard = currentTurn ? 7 : 14;
        uint oldRookSquare = 1ULL << ((longCastle ? 0 : 7) + (currentTurn ? 0 : 56));
        uint newRookSquare = 1ULL << ((longCastle ? 3 : 5) + (!currentTurn ? 0 : 56));
        pieceLocations[rookBitboard] &= ~oldRookSquare; // Update rook bitboard
        pieceLocations[rookBitboard] |= newRookSquare;
    };
    if (promoteTo != 0) { // Handle promotion
        uint promotedToBitboard = promoteTo + 3 + (!currentTurn * 7); 
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

void MoveGen::undoMove(uint move) {
    uint oldSquare = (move & 0b00000000000111111);
    uint newSquare = (move & 0b00000111111000000) >> 6;
    uint captured = (move & 0b00111000000000000) >> 12; // 'capture' order (ascending value): P -> Q -> B -> N -> R -> O-O -> O-O-O
    uint promoteTo = (move & 0b11000000000000000) >> 15; // 'promotion' order (ascending value): Q -> B -> N -> R

    uint relevantBitboard = 16;
    for (uint i = currentTurn ? 2 : 9; currentTurn ? i < 8 : i < 15; i++) {
        if ((1ULL << newSquare) & pieceLocations[i]) {
            relevantBitboard = i;
            break;
        };
    };
    if (0 < captured < 6) { // Handle capturing
        uint opponentBitboard = captured + 1 + (!currentTurn * 7);
        pieceLocations[opponentBitboard] |= (1ULL << newSquare); // Remove from opponent's piece's board
        pieceLocations[currentTurn ? 8 : 1] |= (1ULL << newSquare); // Add to opponent's board
    }
    else if (6 <= captured <= 7) { // Handle castling
        bool longCastle = captured - 6;
        uint rookBitboard = currentTurn ? 7 : 14;
        uint oldRookSquare = 1ULL << ((longCastle ? 0 : 7) + (currentTurn ? 0 : 56));
        uint newRookSquare = 1ULL << ((longCastle ? 3 : 5) + (!currentTurn ? 0 : 56));
        pieceLocations[rookBitboard] &= ~newRookSquare; // Update rook bitboard
        pieceLocations[rookBitboard] |= oldRookSquare;
    };
    if (promoteTo != 0) { // Handle promotion
        uint promotedToBitboard = promoteTo + 3 + (!currentTurn * 7); 
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

bool MoveGen::checksAreValid(uint move) {
    u64 thisKing = pieceLocations[currentTurn ? 2 : 8];
    uint kingSquare = BitOps::countTrailingZeroes(thisKing);
    uint bishopMagicNumber = bishopMagics[kingSquare];
    uint rookMagicNumber = rookMagics[kingSquare];

    u64 enemyPawns = pieceLocations[currentTurn ? 10 : 3];
    u64 enemyQueens = pieceLocations[currentTurn ? 11 : 4];
    u64 enemyBishops = pieceLocations[currentTurn ? 12 : 5];
    u64 enemyKnights = pieceLocations[currentTurn ? 13 : 6];
    u64 enemyRooks = pieceLocations[currentTurn ? 14 : 7];

    uint dangerousBishops = bishopAttacks[kingSquare][(pieceLocations[0] & diagonalMasks[kingSquare]) * bishopMagicNumber];
    uint dangerousKnights = knightMovesTable[kingSquare];
    uint dangerousRooks = rookAttacks[kingSquare][(pieceLocations[0] & cardinalMasks[kingSquare]) * rookMagicNumber];
    uint dangerousPawns = currentTurn ? (thisKing << 9 | thisKing << 7) : (thisKing >> 9 | thisKing >> 7);
    std::vector<uint>::iterator it;
    if (dangerousBishops & (enemyBishops | enemyQueens)) { return false; };
    if (dangerousKnights & enemyKnights) { return false; }; 
    if (dangerousRooks & (enemyRooks | enemyQueens)) { return false; }; 
    if (dangerousPawns & enemyPawns) { return false; };
    return true;
}

std::vector<uint> MoveGen::findKingMoves(uint square) {
    std::vector<uint> kingMoves;
    u64 movesBitboard = kingMovesTable[square];
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        kingMoves.push_back((newSquare << 6) + square);
        movesBitboard ^= (1ULL << newSquare);
    }
    Castles castles = currentTurn ? blackCastles : whiteCastles;
    u64 longCastleMask = 0b00001110;
    u64 shortCastleMask = 0b01100000;
    if (!currentTurn) { longCastleMask << 56; shortCastleMask << 56; };
    if (castles.longCastle & !(longCastleMask & pieceLocations[0])) {
        kingMoves.push_back(((currentTurn ? 2: 58) << 6) + square);
    };
    if (castles.shortCastle & !(shortCastleMask & pieceLocations[0])) {        
        kingMoves.push_back(((currentTurn ? 6: 62) << 6) + square);
    };
    return kingMoves;
}

std::vector<uint> MoveGen::findPawnMoves(u64 bitboard) {
    std::vector<uint> pawnMoves;
    u64 originalRank = currentTurn ? pawnMasks[0] : pawnMasks[1];
    u64 opponentPawns = currentTurn ? pieceLocations[10] : pieceLocations[3];
    u64 pawnPushes = (currentTurn ? ((bitboard & originalRank) << 8) : ((bitboard & originalRank) >> 8)) & ~pieceLocations[0];
    u64 movesBitboard = pawnPushes | (((bitboard) << 8) & ~pieceLocations[0]);
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        pawnMoves.push_back((newSquare << 6) + currentTurn ? newSquare - 8 : newSquare + 8);
        movesBitboard ^= (1ULL << newSquare);
    }
    movesBitboard = currentTurn ? pawnPushes << 8 : pawnPushes >> 8;
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        pawnMoves.push_back((newSquare << 6) + currentTurn ? newSquare - 16 : newSquare + 16);
        movesBitboard ^= (1ULL << newSquare);
    }
    // Handle captures
    movesBitboard = (currentTurn ? (bitboard & edgeMasks[2]) << 7 : (bitboard & edgeMasks[2]) >> 9) & opponentPawns;
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        pawnMoves.push_back((newSquare << 6) + currentTurn ? newSquare - 7 : newSquare + 9);
        movesBitboard ^= (1ULL << newSquare);
    }
    movesBitboard = (currentTurn ? (bitboard & edgeMasks[3]) << 9 : (bitboard & edgeMasks[3]) >> 7) & opponentPawns;
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        pawnMoves.push_back((newSquare << 6) + currentTurn ? newSquare - 9 : newSquare + 7);
        movesBitboard ^= (1ULL << newSquare);
    }

    u64 enPassantCaptures;
    u64 pawnsCanEnPassant = bitboard & pawnMasks[2];
    uint square;
    while (pawnsCanEnPassant != 0) {
        square = BitOps::countTrailingZeroes(pawnsCanEnPassant);
        u64 location = 1ULL << square + 1;
        if (location & pieceLocations[currentTurn ? 10 : 3]); {
            uint newSquare = square + currentTurn ? 9 : -7;
            pawnMoves.push_back((newSquare << 6) + square);
        }
        location >>= 2;
        if (location & pieceLocations[currentTurn ? 10 : 3]); {
            uint newSquare = square + currentTurn ? 7 : -9;
            pawnMoves.push_back((newSquare << 6) + square);
        }
        pawnsCanEnPassant ^= (1ULL << square);
    };
    return pawnMoves;
}

std::vector<uint> MoveGen::findBishopMoves(u64 bitboard) {
    std::vector<uint> bishopMoves;
    uint square = 0;
    while (bitboard != 0) {
        square += BitOps::countTrailingZeroes(bitboard);
        u64 mask = diagonalMasks[square];
        u64 relevantSquares = pieceLocations[0] & mask;
        u64 magicNumber = bishopMagics[square];
        uint shiftIndex = 64 - relevantBitsBishop[square];
        u64 movesBitboard = bishopAttacks[square][(relevantSquares * magicNumber) >> shiftIndex];
        while (movesBitboard > 0) {
            uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
            bishopMoves.push_back((newSquare << 6) + square);
            movesBitboard ^= (1ULL << newSquare);
        }
        bitboard >>= (square + 1);
        square++;
    };
    return bishopMoves;
}

std::vector<uint> MoveGen::findKnightMoves(u64 bitboard) {
    std::vector<uint> knightMoves;
    uint square = 0;
    while (bitboard != 0) {
        square += BitOps::countTrailingZeroes(bitboard);
        u64 movesBitboard = knightMovesTable[square];        
        while (movesBitboard > 0) {
            uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
            knightMoves.push_back((newSquare << 6) + square);
            movesBitboard ^= (1ULL << newSquare);
        }
        bitboard >>= (square + 1);
        square++;
    };
    return knightMoves;
}

std::vector<uint> MoveGen::findRookMoves(u64 bitboard) {
    std::vector<uint> rookMoves;
    uint square = 0;
    while (bitboard != 0) {
        square += BitOps::countTrailingZeroes(bitboard);
        u64 mask = cardinalMasks[square];
        u64 relevantSquares = pieceLocations[0] & mask;
        u64 magicNumber = rookMagics[square];
        uint shiftIndex = 64 - relevantBitsRook[square];
        u64 movesBitboard = rookAttacks[square][(relevantSquares * magicNumber) >> shiftIndex];
        while (movesBitboard > 0) {
            uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
            rookMoves.push_back((newSquare << 6) + square);
            movesBitboard ^= (1ULL << newSquare);
        }
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