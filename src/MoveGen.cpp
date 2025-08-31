
#include "../inc/MoveGen.h"
#include "../inc/Eval.h"
#include "../inc/BitOps.h"
#include <algorithm>

std::vector<uint> MoveGen::findPseudoLegalMoves(Board* board) {
    std::vector<uint> moves;
    std::vector<uint> pseudoLegalMoves;

    u64 pawnLocations = board->pieceLocations[board->currentTurn ? 3 : 10];
    std::vector<uint> pawnMoves = findPawnMoves(board, pawnLocations);

    uint kingSquare = BitOps::countTrailingZeroes(board->pieceLocations[board->currentTurn ? 2 : 9]);
    std::vector<uint> kingMoves = findKingMoves(board, kingSquare);
    // implement castling

    u64 bishopLocations = board->pieceLocations[board->currentTurn ? 5 : 12];
    std::vector<uint> bishopMoves = findBishopMoves(board, bishopLocations);

    u64 knightLocations = board->pieceLocations[board->currentTurn ? 6 : 13];
    std::vector<uint> knightMoves = findKnightMoves(knightLocations);

    u64 rookLocations = board->pieceLocations[board->currentTurn ? 7 : 14];
    std::vector<uint> rookMoves = findRookMoves(board, rookLocations);

    u64 queenLocations = board->pieceLocations[board->currentTurn ? 4 : 12];
    std::vector<uint> queenMoves;
    std::vector<uint> queenDiagonalMoves = findBishopMoves(board, queenLocations);
    std::vector<uint> queenCardinalMoves = findRookMoves(board, queenLocations);
    queenMoves.insert(queenMoves.end(), queenDiagonalMoves.begin(), queenDiagonalMoves.end());
    queenMoves.insert(queenMoves.end(), queenCardinalMoves.begin(), queenCardinalMoves.end());
    
    moves.insert(moves.end(), pawnMoves.begin(), pawnMoves.end());
    std::vector<uint>::iterator it;
    for (it = moves.begin(); it != moves.end(); ++it) {
        uint move = *it;
        uint oldSquare = move & 0b000000111111;
        uint newSquare = (move & 0b111111000000) >> 6;
        // Prevent capturing own pieces
        if ((1ULL << newSquare) & board->pieceLocations[board->currentTurn ? 1 : 8]) { continue; }
        // Handle capturing
        if ((1ULL << newSquare) & board->pieceLocations[board->currentTurn ? 8 : 1]) {
            uint captureValue = 1;
            if ((1ULL << newSquare) & board->pieceLocations[(captureValue + 2) + board->currentTurn ? 8 : 1]) {
                move += (captureValue << 12);
            }
        }
        // Handle promotion
        if (oldSquare & board->pieceLocations[board->currentTurn ? 3 : 10] && newSquare >= 56) {
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

std::vector<MoveGen::MoveData> MoveGen::findLegalMoves(Board* board, std::vector<uint> moveList) {
    std::vector<MoveGen::MoveData> legalMoves;
    std::vector<uint>::iterator it;
    for(it = moveList.begin(); it != moveList.end(); ++it) {
        uint move = *it;
        doMove(board, move);
        if (checksAreValid(board, move)) {
            MoveData moveData;
            moveData.move = move;
            moveData.score = Eval::calculateMoveOrderScore(move);
            legalMoves.push_back(moveData);
        }
        undoMove(board, move);
    }
    std::sort(legalMoves.begin(), legalMoves.end(), Eval::compareByScore);
    return legalMoves;
}

Board* MoveGen::doMove(Board* board, uint move) {
    Board newBoard = *board;
    uint oldSquare = (move & 0b00000000000111111);
    uint newSquare = (move & 0b00000111111000000) >> 6;
    uint captured = (move & 0b00111000000000000) >> 12; // 'capture' order (ascending value): P -> Q -> B -> N -> R -> O-O -> O-O-O
    uint promoteTo = (move & 0b11000000000000000) >> 15; // 'promotion' order (ascending value): Q -> B -> N -> R

    uint relevantBitboard = 16;
    for (uint i = board->currentTurn ? 2 : 9; board->currentTurn ? i < 8 : i < 15; i++) {
        if ((1ULL << oldSquare) & board->pieceLocations[i]) {
            relevantBitboard = i;
            break;
        };
    };
    if (0 < captured < 6) { // Handle capturing
        uint opponentBitboard = captured + 1 + (!board->currentTurn * 7);
        newBoard.pieceLocations[opponentBitboard] &= ~(1ULL << newSquare); // Remove from opponent's piece's board
        newBoard.pieceLocations[board->currentTurn ? 8 : 1] &= ~(1ULL << newSquare); // Remove from opponent's board
    }
    else if (6 <= captured <= 7) { // Handle castling
        bool longCastle = captured - 6;
        uint rookBitboard = board->currentTurn ? 7 : 14;
        uint oldRookSquare = 1ULL << ((longCastle ? 0 : 7) + (board->currentTurn ? 0 : 56));
        uint newRookSquare = 1ULL << ((longCastle ? 3 : 5) + (!board->currentTurn ? 0 : 56));
        newBoard.pieceLocations[rookBitboard] &= ~oldRookSquare; // Update rook bitboard
        newBoard.pieceLocations[rookBitboard] |= newRookSquare;
    };
    if (promoteTo != 0) { // Handle promotion
        uint promotedToBitboard = promoteTo + 3 + (!board->currentTurn * 7); 
        newBoard.pieceLocations[promotedToBitboard] |= (1ULL << newSquare); // Add to new piece's bitboard
        relevantBitboard = 16; // Don't update pawn bitboard with new location
    }

    // Add to bitboard for piece type
    try {
        newBoard.pieceLocations[relevantBitboard] & ~(1ULL << oldSquare);
        newBoard.pieceLocations[relevantBitboard] |= (1ULL << newSquare);
    } catch (const std::out_of_range &e) {};

    // Add to bitboard for all pieces 
    newBoard.pieceLocations[0] & ~(1ULL << oldSquare);
    newBoard.pieceLocations[0] |= (1ULL << newSquare);
    
    // Add to bitboard for piece's colour
    newBoard.pieceLocations[board->currentTurn ? 1 : 8] & ~(1ULL << oldSquare);
    newBoard.pieceLocations[board->currentTurn ? 1 : 8] |= (1ULL << newSquare);
    
    newBoard.currentTurn = !newBoard.currentTurn;
    return &newBoard;
}

Board* MoveGen::undoMove(Board* board, uint move) {
    Board newBoard = *board;
    uint oldSquare = (move & 0b00000000000111111);
    uint newSquare = (move & 0b00000111111000000) >> 6;
    uint captured = (move & 0b00111000000000000) >> 12; // 'capture' order (ascending value): P -> Q -> B -> N -> R -> O-O -> O-O-O
    uint promoteTo = (move & 0b11000000000000000) >> 15; // 'promotion' order (ascending value): Q -> B -> N -> R

    uint relevantBitboard = 16;
    for (uint i = board->currentTurn ? 2 : 9; board->currentTurn ? i < 8 : i < 15; i++) {
        if ((1ULL << newSquare) & board->pieceLocations[i]) {
            relevantBitboard = i;
            break;
        };
    };
    if (0 < captured < 6) { // Handle capturing
        uint opponentBitboard = captured + 1 + (!board->currentTurn * 7);
        newBoard.pieceLocations[opponentBitboard] |= (1ULL << newSquare); // Remove from opponent's piece's board
        newBoard.pieceLocations[board->currentTurn ? 8 : 1] |= (1ULL << newSquare); // Add to opponent's board
    }
    else if (6 <= captured <= 7) { // Handle castling
        bool longCastle = captured - 6;
        uint rookBitboard = board->currentTurn ? 7 : 14;
        uint oldRookSquare = 1ULL << ((longCastle ? 0 : 7) + (board->currentTurn ? 0 : 56));
        uint newRookSquare = 1ULL << ((longCastle ? 3 : 5) + (!board->currentTurn ? 0 : 56));
        newBoard.pieceLocations[rookBitboard] &= ~newRookSquare; // Update rook bitboard
        newBoard.pieceLocations[rookBitboard] |= oldRookSquare;
    };
    if (promoteTo != 0) { // Handle promotion
        uint promotedToBitboard = promoteTo + 3 + (!board->currentTurn * 7); 
        newBoard.pieceLocations[promotedToBitboard] &= ~(1ULL << newSquare); // Remove from new piece's bitboard
        relevantBitboard = board->currentTurn ? 3 : 10;
    }

    // Remove from bitboard for piece type
    try {
        newBoard.pieceLocations[relevantBitboard] & ~(1ULL << newSquare);
        newBoard.pieceLocations[relevantBitboard] |= (1ULL << oldSquare);
    } catch (const std::out_of_range &e) {};

    // Add to bitboard for all pieces
    if (!captured) { board->pieceLocations[0] & ~(1ULL << newSquare); }
    newBoard.pieceLocations[0] |= (1ULL << newSquare);
    
    // Add to bitboard for piece's colour
    newBoard.pieceLocations[board->currentTurn ? 1 : 8] & ~(1ULL << oldSquare);
    newBoard.pieceLocations[board->currentTurn ? 1 : 8] |= (1ULL << newSquare);

    newBoard.currentTurn = !newBoard.currentTurn;
    return &newBoard;
}

bool MoveGen::checksAreValid(Board* board, uint move) {
    u64 thisKing = board->pieceLocations[board->currentTurn ? 2 : 8];
    uint kingSquare = BitOps::countTrailingZeroes(thisKing);
    uint bishopMagicNumber = bishopMagics[kingSquare];
    uint rookMagicNumber = rookMagics[kingSquare];

    u64 enemyPawns = board->pieceLocations[board->currentTurn ? 10 : 3];
    u64 enemyQueens = board->pieceLocations[board->currentTurn ? 11 : 4];
    u64 enemyBishops = board->pieceLocations[board->currentTurn ? 12 : 5];
    u64 enemyKnights = board->pieceLocations[board->currentTurn ? 13 : 6];
    u64 enemyRooks = board->pieceLocations[board->currentTurn ? 14 : 7];

    uint dangerousBishops = bishopAttacks[kingSquare][(board->pieceLocations[0] & diagonalMasks[kingSquare]) * bishopMagicNumber];
    uint dangerousKnights = knightMovesTable[kingSquare];
    uint dangerousRooks = rookAttacks[kingSquare][(board->pieceLocations[0] & cardinalMasks[kingSquare]) * rookMagicNumber];
    uint dangerousPawns = board->currentTurn ? (thisKing << 9 | thisKing << 7) : (thisKing >> 9 | thisKing >> 7);
    std::vector<uint>::iterator it;
    if (dangerousBishops & (enemyBishops | enemyQueens)) { return false; };
    if (dangerousKnights & enemyKnights) { return false; }; 
    if (dangerousRooks & (enemyRooks | enemyQueens)) { return false; }; 
    if (dangerousPawns & enemyPawns) { return false; };
    return true;
}

std::vector<uint> MoveGen::findKingMoves(Board* board, uint square) {
    std::vector<uint> kingMoves;
    u64 movesBitboard = kingMovesTable[square];
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        kingMoves.push_back((newSquare << 6) + square);
        movesBitboard ^= (1ULL << newSquare);
    }
    Board::Castles castles = board->currentTurn ? board->blackCastles : board->whiteCastles;
    u64 longCastleMask = 0b00001110;
    u64 shortCastleMask = 0b01100000;
    if (!board->currentTurn) { longCastleMask << 56; shortCastleMask << 56; };
    if (castles.longCastle & !(longCastleMask & board->pieceLocations[0])) {
        kingMoves.push_back(((board->currentTurn ? 2: 58) << 6) + square);
    };
    if (castles.shortCastle & !(shortCastleMask & board->pieceLocations[0])) {        
        kingMoves.push_back(((board->currentTurn ? 6: 62) << 6) + square);
    };
    return kingMoves;
}

std::vector<uint> MoveGen::findPawnMoves(Board* board, u64 bitboard) {
    std::vector<uint> pawnMoves;
    u64 originalRank = board->currentTurn ? pawnMasks[0] : pawnMasks[1];
    u64 opponentPawns = board->currentTurn ? board->pieceLocations[10] : board->pieceLocations[3];
    u64 pawnPushes = (board->currentTurn ? ((bitboard & originalRank) << 8) : ((bitboard & originalRank) >> 8)) & ~board->pieceLocations[0];
    u64 movesBitboard = pawnPushes | (((bitboard) << 8) & ~board->pieceLocations[0]);
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        pawnMoves.push_back((newSquare << 6) + board->currentTurn ? newSquare - 8 : newSquare + 8);
        movesBitboard ^= (1ULL << newSquare);
    }
    movesBitboard = board->currentTurn ? pawnPushes << 8 : pawnPushes >> 8;
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        pawnMoves.push_back((newSquare << 6) + board->currentTurn ? newSquare - 16 : newSquare + 16);
        movesBitboard ^= (1ULL << newSquare);
    }
    // Handle captures
    movesBitboard = (board->currentTurn ? (bitboard & edgeMasks[2]) << 7 : (bitboard & edgeMasks[2]) >> 9) & opponentPawns;
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        pawnMoves.push_back((newSquare << 6) + board->currentTurn ? newSquare - 7 : newSquare + 9);
        movesBitboard ^= (1ULL << newSquare);
    }
    movesBitboard = (board->currentTurn ? (bitboard & edgeMasks[3]) << 9 : (bitboard & edgeMasks[3]) >> 7) & opponentPawns;
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        pawnMoves.push_back((newSquare << 6) + board->currentTurn ? newSquare - 9 : newSquare + 7);
        movesBitboard ^= (1ULL << newSquare);
    }

    u64 enPassantCaptures;
    u64 pawnsCanEnPassant = bitboard & pawnMasks[2];
    uint square;
    while (pawnsCanEnPassant != 0) {
        square = BitOps::countTrailingZeroes(pawnsCanEnPassant);
        u64 location = 1ULL << square + 1;
        if (location & board->pieceLocations[board->currentTurn ? 10 : 3]); {
            uint newSquare = square + board->currentTurn ? 9 : -7;
            pawnMoves.push_back((newSquare << 6) + square);
        }
        location >>= 2;
        if (location & board->pieceLocations[board->currentTurn ? 10 : 3]); {
            uint newSquare = square + board->currentTurn ? 7 : -9;
            pawnMoves.push_back((newSquare << 6) + square);
        }
        pawnsCanEnPassant ^= (1ULL << square);
    };
    return pawnMoves;
}

std::vector<uint> MoveGen::findBishopMoves(Board* board, u64 bitboard) {
    std::vector<uint> bishopMoves;
    uint square = 0;
    while (bitboard != 0) {
        square += BitOps::countTrailingZeroes(bitboard);
        u64 mask = diagonalMasks[square];
        u64 relevantSquares = board->pieceLocations[0] & mask;
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

std::vector<uint> MoveGen::findRookMoves(Board* board, u64 bitboard) {
    std::vector<uint> rookMoves;
    uint square = 0;
    while (bitboard != 0) {
        square += BitOps::countTrailingZeroes(bitboard);
        u64 mask = cardinalMasks[square];
        u64 relevantSquares = board->pieceLocations[0] & mask;
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