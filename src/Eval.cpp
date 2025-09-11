#include "../inc/Eval.h"
#include <algorithm>

std::vector<uint> Eval::findPseudoLegalMoves() {
    std::vector<uint> moves;
    std::vector<uint> pseudoLegalMoves;
    
    u64 pawnLocations = board->pieceLocations[board->currentTurn ? 3 : 10];
    std::vector<uint> pawnMoves = findPawnMoves(pawnLocations);
    
    uint kingSquare = BitOps::countTrailingZeroes(board->pieceLocations[board->currentTurn ? 2 : 9]);
    std::vector<uint> kingMoves = findKingMoves(kingSquare);
    // implement castling
    
    u64 bishopLocations = board->pieceLocations[board->currentTurn ? 4 : 11];
    std::vector<uint> bishopMoves = findBishopMoves(bishopLocations);
    
    u64 knightLocations = board->pieceLocations[board->currentTurn ? 5 : 12];
    std::vector<uint> knightMoves = findKnightMoves(knightLocations);
    
    u64 rookLocations = board->pieceLocations[board->currentTurn ? 6 : 13];
    std::vector<uint> rookMoves = findRookMoves(rookLocations);
    
    u64 queenLocations = board->pieceLocations[board->currentTurn ? 7 : 14];
    std::vector<uint> queenMoves;
    std::vector<uint> queenDiagonalMoves = findBishopMoves(queenLocations);
    std::vector<uint> queenCardinalMoves = findRookMoves(queenLocations);
    queenMoves.insert(queenMoves.end(), queenDiagonalMoves.begin(), queenDiagonalMoves.end());
    queenMoves.insert(queenMoves.end(), queenCardinalMoves.begin(), queenCardinalMoves.end());
    
    moves.insert(moves.end(), pawnMoves.begin(), pawnMoves.end());
    moves.insert(moves.end(), bishopMoves.begin(), bishopMoves.end());
    moves.insert(moves.end(), knightMoves.begin(), knightMoves.end());
    moves.insert(moves.end(), rookMoves.begin(), rookMoves.end());
    moves.insert(moves.end(), queenMoves.begin(), queenMoves.end());
    moves.insert(moves.end(), kingMoves.begin(), kingMoves.end());

    std::vector<uint>::iterator it;
    for (it = moves.begin(); it != moves.end(); ++it) {
        uint move = *it;
        uint oldSquare = move & 0b000000111111;
        uint newSquare = (move & 0b111111000000) >> 6;
        // Prevent capturing own pieces
        if ((1ULL << newSquare) & board->pieceLocations[board->currentTurn ? 1 : 8]) { continue; }
        // Handle capturing
        if ((1ULL << newSquare) & board->pieceLocations[board->currentTurn ? 8 : 1]) {
            for (int captureValue = 0; captureValue < 5; captureValue++)
            if ((1ULL << newSquare) & board->pieceLocations[(captureValue + 2) + board->currentTurn ? 8 : 1]) {
                move += (captureValue << 12);
                break;
            }
        }
        // Handle promotion
        if (oldSquare & board->pieceLocations[board->currentTurn ? 3 : 10] && newSquare >= 56) {
            // Iterate over each possible promotion
            for (int promotionValue = 1; promotionValue < 4; promotionValue++){
                move += (promotionValue << 15);
                pseudoLegalMoves.push_back(move);
            }
            continue;
        }
        pseudoLegalMoves.push_back(move);
    }
    return pseudoLegalMoves;
    
}

std::vector<MoveData*> Eval::findLegalMoves(std::vector<uint> moveList) {
    std::vector<MoveData*> legalMoves;
    std::vector<uint>::iterator it;
    for(it = moveList.begin(); it != moveList.end(); ++it) {
        uint move = *it;
    }
    for(it = moveList.begin(); it != moveList.end(); ++it) {
        uint move = *it;
        doMove(move);
        if (bool valid = checksAreValid(move)) {
            board->currentTurn = !board->currentTurn;
            bool check = checksAreValid(move);
            MoveData* moveData = new MoveData(move, check);
            this->calculateMoveOrderScore(moveData);

            board->currentTurn = !board->currentTurn;
            Board* tmpBoard = board;
            legalMoves.push_back(moveData);
        }
        undoMove(move);
    }
    std::sort(legalMoves.begin(), legalMoves.end(), compareByScore);
    return legalMoves;
}

std::vector<uint> Eval::findKingMoves(uint square) {
    std::vector<uint> kingMoves;
    u64 movesBitboard = kingMovesTable[square];
    while (movesBitboard > 0) {
        uint newSquare = BitOps::countTrailingZeroes(movesBitboard);
        kingMoves.push_back((newSquare << 6) + square);
        movesBitboard ^= (1ULL << newSquare);
    }
    Castles castles = board->currentTurn ? board->blackCastles : board->whiteCastles;
    u64 longCastleMask = 0b00001110;
    u64 shortCastleMask = 0b01100000;
    if (!board->currentTurn) { longCastleMask <<= 56; shortCastleMask <<= 56; };
    if (castles.longCastle & !(longCastleMask & board->pieceLocations[0])) {
        kingMoves.push_back(((board->currentTurn ? 2: 58) << 6) + square);
    };
    if (castles.shortCastle & !(shortCastleMask & board->pieceLocations[0])) {        
        kingMoves.push_back(((board->currentTurn ? 6: 62) << 6) + square);
    };
    return kingMoves;
}

std::vector<uint> Eval::findPawnMoves(u64 bitboard) {
    std::vector<uint> pawnMoves;
    u64 firstPushRank = board->currentTurn ? pawnMasks[0] : pawnMasks[1];
    u64 opponentPieces = board->currentTurn ? board->pieceLocations[8] : board->pieceLocations[1];
    while (bitboard != 0) {
        u64 enPassantBitboard = board->enPassantFiles << (board->currentTurn ? 40 : 16);
        uint oldSquareIndex = BitOps::countTrailingZeroes(bitboard);
        u64 oldSquare = 1ULL << oldSquareIndex;
        u64 newSquares = 0;
    
        newSquares |= ~(board->pieceLocations[0]) & (board->currentTurn ? oldSquare << 8 : oldSquare >> 8); // First push
        newSquares |= ~(board->pieceLocations[0]) & (board->currentTurn ? (firstPushRank & newSquares) << 8 : (firstPushRank & newSquares) >> 8); // Double push
        newSquares |= (opponentPieces | enPassantBitboard) & (board->currentTurn ? (oldSquare & edgeMasks[2]) << 7 : (oldSquare & edgeMasks[2]) >> 9); // Captures right
        newSquares |= (opponentPieces | enPassantBitboard) & (board->currentTurn ? (oldSquare & edgeMasks[3]) << 9 : (oldSquare & edgeMasks[2]) >> 7); // Captures left

        while (newSquares != 0) {
            uint newSquare = BitOps::countTrailingZeroes(newSquares);
            pawnMoves.push_back((newSquare << 6) + oldSquareIndex);
            newSquares ^= (1ULL << newSquare);
        } 
        bitboard ^= (1ULL << oldSquareIndex);
    }
    return pawnMoves;
}

std::vector<uint> Eval::findBishopMoves(u64 bitboard) {
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

std::vector<uint> Eval::findKnightMoves(u64 bitboard) {
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

std::vector<uint> Eval::findRookMoves(u64 bitboard) {
    std::vector<uint> rookMoves;
    uint square = 0;
    while (bitboard != 0) {
        square = BitOps::countTrailingZeroes(bitboard);
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
        bitboard ^= 1ULL << square;
    };
    return rookMoves;
}

uint Eval::calculateMagicHash(uint square, MagicPiece bishop, u64 occupancy) {
    uint hash;
    u64 magicNumber = bishop ? this->bishopMagics[square] : this->rookMagics[square];
    u64 mask = bishop ? this->diagonalMasks[square] : this->cardinalMasks[square];
    uint relevantBits = bishop ? this->relevantBitsBishop[square] : this->relevantBitsRook[square]; 
    u64 blockers = occupancy & mask;
    hash = (uint)((blockers * magicNumber) >> (64 - relevantBits));
    return hash;
}

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

float Eval::evaluatePosition() {
    const float PIECEVALUES[7] = {0, 0, 1, 3, 3, 5, 9};
    float score = 0;
    for (int i = 3; i < 15; i++) {
        // Assess relative piece value
        if (i == 8 || i == 9) { continue; }
        u64 bitboard = board->pieceLocations[i];
        score += BitOps::countSetBits(bitboard) * PIECEVALUES[((i-1)  % 7)];
        // while (bitboard > 0) {
        //     uint square = BitOps::countTrailingZeroes(bitboard);
        //     float multiplier = DEFAULT_VALUE_MODIFIER;
        //     if (i == 5 || i == 12) { HeatMaps::knight[square]; };
        //     float pieceValue = PIECEVALUES[((i-1)  % 7)] * (i < 8 ? 1 : -1);
        //     score += pieceValue * multiplier;
        //     bitboard ^= 1ULL << square;
        // }
    }
    // Assess pawn structure
    // u64 whitePawns = board->pieceLocations[3];
    // u64 blackPawns = board->pieceLocations[10];
    // u64 whitePawnChains = ((whitePawns & edgeMasks[2]) << 7 || (whitePawns & edgeMasks[3]) << 9) & whitePawns;
    // u64 blackPawnChains = ((blackPawns & edgeMasks[3]) >> 7 || (blackPawns & edgeMasks[2]) >> 9) & blackPawns;
    // score += (float)PAWN_CHAIN_VALUE * BitOps::countSetBits(whitePawnChains);
    // score -= (float)PAWN_CHAIN_VALUE * BitOps::countSetBits(blackPawnChains);
    // u64 whitePawnStacks = 0;
    // u64 blackPawnStacks = 0;
    // for (uint rank = 1; rank < 8; ++rank) {
    //     whitePawnStacks |= (whitePawns << rank * 8) & whitePawns;
    //     blackPawnStacks |= (blackPawns >> rank * 8) & blackPawns;
    // }
    // score += (float)PAWN_STACK_VALUE * BitOps::countSetBits(blackPawnStacks);
    // score -= (float)PAWN_STACK_VALUE * BitOps::countSetBits(whitePawnStacks);

    // Assess whether favourable captures exist (attack/defend bitboards)
    return score;
};

float Eval::evalAlphaBeta(uint depth, float alpha, float beta) {
    if (depth == 0) { float score = evaluatePosition(); return score; };
    std::vector<MoveData*> moves = findLegalMoves(findPseudoLegalMoves());
    std::vector<MoveData*>::iterator iterator;
    for (iterator = moves.begin(); iterator != moves.end(); ++iterator) {
        uint move = (*iterator)->move;
    }
    if (moves.size() == 0) {
        return INFINITY * board->currentTurn ? 1.0f : -1.0f;
    };
    if (float eval = checkTransposition(board->zobristHash, depth, alpha, beta) != INVALID_TRANSPOSITION_EVAL) {
        return eval;
    }
    if (board->currentTurn) {
        float eval = -INFINITY;
        std::vector<MoveData*>::iterator it;
        MoveData* storeMove;
        NodeType tpNodeType;
        for (it = moves.begin(); it != moves.end(); ++it) {
            MoveData move = **it;
            doMove(move.move);
            eval = std::max(eval, evalAlphaBeta((depth - 1), alpha, beta));
            if (eval >= beta) {
                killers[currentDepth].addNewKiller(move.move);
                storeMove = &move;
                tpNodeType = BETA;
                undoMove(move.move);
                total++;
                break;
            }
            else if (eval > alpha) {
                alpha = eval;
                storeMove = &move;
                tpNodeType = EXACT;
            }
            else {
                storeMove = &move;
                tpNodeType = ALPHA;
            }
            total++;
            undoMove(move.move);
        }
        Transposition tp;
        tp.init(board->zobristHash, storeMove, depth, eval, tpNodeType);
        addTransposition(tp);
        return eval;
    }
    else {
        float eval = INFINITY;
        std::vector<MoveData*>::iterator it;
        MoveData* storeMove;
        NodeType tpNodeType;
        for (it = moves.begin(); it != moves.end(); ++it) {
            MoveData move = **it;
            doMove(move.move);
            eval = std::min(eval, evalAlphaBeta(depth - 1, alpha, beta));
            if (eval <= alpha) {
                killers[currentDepth].addNewKiller(move.move);
                storeMove = &move;
                tpNodeType = ALPHA;
                undoMove(move.move);
                total++;
                break;
            }
            else if (eval < beta) {
                beta = eval;
                storeMove = &move;
                tpNodeType = EXACT;
            }
            else {
                storeMove = &move;
                tpNodeType = BETA;
            }
            total++;
            undoMove(move.move);
        }
        Transposition tp;
        tp.init(board->zobristHash, storeMove, depth, eval, tpNodeType);
        addTransposition(tp);
        return eval;
    }
        
};

void Eval::doMove(uint move) {
    uint oldSquare = (move & 0b000000000000111111);
    uint newSquare = (move & 0b000000111111000000) >> 6;
    uint captured = (move & 0b000111000000000000) >> 12; // 'capture' order (ascending value): None -> P -> B -> N -> R -> -> Q -> O-O -> O-O-O
    uint promoteTo = (move & 0b111000000000000000) >> 15; // 'promotion' order (ascending value): None -> B -> N -> R -> Q

    uint relevantBitboard = 16; // Out of range by default
    for (uint i = board->currentTurn ? 2 : 9; board->currentTurn ? i < 8 : i < 15; i++) {
        if ((1ULL << oldSquare) & board->pieceLocations[i]) {
            relevantBitboard = i;
            break;
        };
    };
    if (captured > 0 && captured < 6) { // Handle capturing
        uint opponentBitboard = captured + 1 + (!board->currentTurn * 7);
        board->pieceLocations[opponentBitboard] &= ~(1ULL << newSquare); // Remove from opponent's piece's board
        board->pieceLocations[board->currentTurn ? 8 : 1] &= ~(1ULL << newSquare); // Remove from opponent's board
        // Update Zobrist hash
        u64 zobristNumber = board->zobristPseudoRandoms[12 * newSquare + (opponentBitboard - (2 + opponentBitboard > 7 ? 1 : 0))];
        board->zobristHash^=zobristNumber;

    }
    else if (captured > 0 && captured < 8) { // Handle castling
        bool longCastle = captured - 6;
        uint rookBitboard = board->currentTurn ? 6 : 13;
        uint oldRookSquare = 1ULL << ((longCastle ? 0 : 7) + (board->currentTurn ? 0 : 56));
        uint newRookSquare = 1ULL << ((longCastle ? 3 : 5) + (!board->currentTurn ? 0 : 56));
        uint removeOldRookSqaure = ~oldRookSquare;
        board->pieceLocations[rookBitboard] &= removeOldRookSqaure; // Update rook bitboard
        board->pieceLocations[rookBitboard] |= newRookSquare;
        // Update Zobrist hash
        u64 zobristNumberRemove = board->zobristPseudoRandoms[12 * oldRookSquare + (rookBitboard - (2 + rookBitboard > 7 ? 1 : 0))];
        u64 zobristNumberAdd = board->zobristPseudoRandoms[12 * newRookSquare + (rookBitboard - (2 + rookBitboard > 7 ? 1 : 0))];
        board->zobristHash^=zobristNumberRemove;
        board->zobristHash^=zobristNumberAdd;

    };
    if (promoteTo != 0) { // Handle promotion
        uint promotedToBitboard = promoteTo + 3 + (!board->currentTurn * 7); 
        board->pieceLocations[promotedToBitboard] |= (1ULL << newSquare); // Add to new piece's bitboard
        relevantBitboard = 16; // Don't update pawn bitboard with new location
        // Update Zobrist Hash
        u64 zobristNumber = board->zobristPseudoRandoms[12 * newSquare + (promotedToBitboard - (2 + promotedToBitboard > 7 ? 1 : 0))];
        board->zobristHash^=zobristNumber;
    }

    // Add to bitboard for piece type
    try {
        board->pieceLocations[relevantBitboard] &= ~(1ULL << oldSquare);
        board->pieceLocations[relevantBitboard] |= (1ULL << newSquare);
        // Update Zobrist hash
        u64 zobristNumberRemove = board->zobristPseudoRandoms[12 * oldSquare + (relevantBitboard - (2 + relevantBitboard > 7 ? 1 : 0))];
        u64 zobristNumberAdd = board->zobristPseudoRandoms[12 * newSquare + (relevantBitboard - (2 + relevantBitboard > 7 ? 1 : 0))];
        board->zobristHash^=zobristNumberRemove;
        board->zobristHash^=zobristNumberAdd;
    } catch (const std::out_of_range) {};

    // Add to bitboard for all pieces 
    board->pieceLocations[0] &= ~(1ULL << oldSquare);
    board->pieceLocations[0] |= (1ULL << newSquare);
    
    // Add to bitboard for piece's colour
    board->pieceLocations[board->currentTurn ? 1 : 8] &= ~(1ULL << oldSquare);
    board->pieceLocations[board->currentTurn ? 1 : 8] |= (1ULL << newSquare);
    
    board->currentTurn = !board->currentTurn;
}

void Eval::undoMove(uint move) {
    board->currentTurn = !board->currentTurn;

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
    if (captured > 0 && captured < 6) { // Handle capturing
        uint opponentBitboard = captured + 1 + (board->currentTurn * 7);
        board->pieceLocations[opponentBitboard] |= (1ULL << newSquare); // Add to opponent's piece's board
        board->pieceLocations[board->currentTurn ? 8 : 1] |= (1ULL << newSquare); // Add to opponent's board
    }
    else if (captured > 5 && captured < 8) { // Handle castling
        bool longCastle = captured - 6;
        u64 rookBitboard = board->currentTurn ? 7 : 14;
        u64 oldRookSquare = 1ULL << ((longCastle ? 0 : 7) + (board->currentTurn ? 0 : 56));
        u64 newRookSquare = 1ULL << ((longCastle ? 3 : 5) + (!board->currentTurn ? 0 : 56));
        board->pieceLocations[rookBitboard] &= ~newRookSquare; // Update rook bitboard
        board->pieceLocations[rookBitboard] |= oldRookSquare;
        // Update Zobrist hash
        u64 zobristNumberRemove = board->zobristPseudoRandoms[12 * newRookSquare + (rookBitboard - (2 + rookBitboard > 7 ? 1 : 0))];
        u64 zobristNumberAdd = board->zobristPseudoRandoms[12 * oldRookSquare + (rookBitboard - (2 + rookBitboard > 7 ? 1 : 0))];
        board->zobristHash^=zobristNumberRemove;
        board->zobristHash^=zobristNumberAdd;
    };
    if (promoteTo != 0) { // Handle promotion
        uint promotedToBitboard = promoteTo + 3 + (!board->currentTurn * 7); 
        board->pieceLocations[promotedToBitboard] &= ~(1ULL << newSquare); // Remove from new piece's bitboard
        relevantBitboard = board->currentTurn ? 3 : 10;
        // Update Zobrist hash
        u64 zobristNumberRemove = board->zobristPseudoRandoms[12 * newSquare + (promotedToBitboard - (2 + promotedToBitboard > 7 ? 1 : 0))];
        board->zobristHash^=zobristNumberRemove;
    }

    // Remove from bitboard for piece type
    try {
        board->pieceLocations[relevantBitboard] &= ~(1ULL << newSquare);
        board->pieceLocations[relevantBitboard] |= (1ULL << oldSquare);
        // Update Zobrist hash
        u64 zobristNumberRemove = board->zobristPseudoRandoms[12 * newSquare + (relevantBitboard - (2 + relevantBitboard > 7 ? 1 : 0))];
        u64 zobristNumberAdd = board->zobristPseudoRandoms[12 * oldSquare + (relevantBitboard- (2 + relevantBitboard > 7 ? 1 : 0))];
        board->zobristHash^=zobristNumberRemove;
        board->zobristHash^=zobristNumberAdd;
    } catch (const std::out_of_range) {};

    // Add to bitboard for all pieces
    if (!captured) { board->pieceLocations[0] &= ~(1ULL << newSquare); }
    board->pieceLocations[0] |= (1ULL << oldSquare);
    
    // Add to bitboard for piece's colour
    board->pieceLocations[board->currentTurn ? 1 : 8] &= ~(1ULL << newSquare);
    board->pieceLocations[board->currentTurn ? 1 : 8] |= (1ULL << oldSquare);
}

void Eval::calculateMoveOrderScore(MoveData* moveData) {
    const int PIECEVALUES[7] = {0, 0, 100, 300, 300, 500, 900};
    float score = 0;
    // 1 refutation from TT (handled elsewhere)
    // 2 killer moves
    if (moveData->move == this->killers[currentDepth].first) { moveData->score = INFINITY - 1; }
    else if (moveData->move == killers[currentDepth].second) { moveData->score = INFINITY - 2; };
    // 3 captures in order of risk/reward
    uint capture = (moveData->move & 0b00111000000000000) >> 12;
    if (capture) {
        if (capture >= 6) {
            score += capture;
        }
        else {            
            score += PIECEVALUES[capture+1] * 2;
        }
    };
    // 4 checks
    if (moveData->check) { score += 1; };
    // 5 promotions
    uint promotion = (moveData->move & 0b11000000000000000) >> 15;
    if (promotion) {
        score += PIECEVALUES[promotion + 2];
    }
    moveData->score = score;
};

bool Eval::checksAreValid(uint move) {
    // TODO: add clause for castling
    u64 thisKing = board->pieceLocations[board->currentTurn ? 9 : 2];
    uint kingSquare = BitOps::countTrailingZeroes(thisKing);
    u64 bishopMagicNumber = this->bishopMagics[kingSquare];
    u64 rookMagicNumber = this->rookMagics[kingSquare];
    u64 enemyKing = board->pieceLocations[board->currentTurn ? 2 : 9];
    u64 enemyPawns = board->pieceLocations[board->currentTurn ? 3 : 10];
    u64 enemyBishops = board->pieceLocations[board->currentTurn ? 4 : 11];
    u64 enemyKnights = board->pieceLocations[board->currentTurn ? 5 : 12];
    u64 enemyRooks = board->pieceLocations[board->currentTurn ? 6 : 13];
    u64 enemyQueens = board->pieceLocations[board->currentTurn ? 7 : 14];

    u64 dangerousKingLocations = kingMovesTable[kingSquare];
    u64 dangerousBishopsLocations = bishopAttacks[kingSquare][calculateMagicHash(kingSquare, BISHOP, board->pieceLocations[0])];
    u64 dangerousKnightsLocations = knightMovesTable[kingSquare];
    u64 dangerousRooksLocations = rookAttacks[kingSquare][calculateMagicHash(kingSquare, ROOK, board->pieceLocations[0])];
    u64 dangerousPawnsLocations = board->currentTurn ? ((thisKing & edgeMasks[3]) << 9 | (thisKing & edgeMasks[2]) << 7) : ((thisKing & edgeMasks[2]) >> 9 | (thisKing & edgeMasks[3]) >> 7);
    if (dangerousKingLocations & enemyKing) { return false; };
    if (dangerousBishopsLocations & (enemyBishops | enemyQueens)) { return false; };
    if (dangerousKnightsLocations & enemyKnights) { return false; };
    if (dangerousRooksLocations & (enemyRooks | enemyQueens)) { return false; };
    if (dangerousPawnsLocations & enemyPawns) { return false; };
    return true;
}
