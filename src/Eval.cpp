#include "../inc/Eval.h"
#include <algorithm>

std::vector<MoveData*>Eval::findPseudoLegalMoves() {
    std::vector<MoveData*>moves;
    std::vector<MoveData*>pseudoLegalMoves;
    
    u64 pawnLocations = board->pieceLocations[board->currentTurn ? 3 : 10];
    std::vector<MoveData*>pawnMoves = findPawnMoves(pawnLocations);
    
    uint kingSquare = BitOps::countTrailingZeroes(board->pieceLocations[board->currentTurn ? 2 : 9]);
    std::vector<MoveData*>kingMoves = findKingMoves(kingSquare);
    
    u64 bishopLocations = board->pieceLocations[board->currentTurn ? 4 : 11];
    std::vector<MoveData*>bishopMoves = findBishopMoves(bishopLocations);
    
    u64 knightLocations = board->pieceLocations[board->currentTurn ? 5 : 12];
    std::vector<MoveData*>knightMoves = findKnightMoves(knightLocations);
    
    u64 rookLocations = board->pieceLocations[board->currentTurn ? 6 : 13];
    std::vector<MoveData*>rookMoves = findRookMoves(rookLocations);
    
    u64 queenLocations = board->pieceLocations[board->currentTurn ? 7 : 14];
    std::vector<MoveData*>queenMoves;
    std::vector<MoveData*>queenDiagonalMoves = findBishopMoves(queenLocations, true);
    std::vector<MoveData*>queenCardinalMoves = findRookMoves(queenLocations, true);
    queenMoves.insert(queenMoves.end(), queenDiagonalMoves.begin(), queenDiagonalMoves.end());
    queenMoves.insert(queenMoves.end(), queenCardinalMoves.begin(), queenCardinalMoves.end());
    
    moves.insert(moves.end(), pawnMoves.begin(), pawnMoves.end());
    moves.insert(moves.end(), bishopMoves.begin(), bishopMoves.end());
    moves.insert(moves.end(), knightMoves.begin(), knightMoves.end());
    moves.insert(moves.end(), rookMoves.begin(), rookMoves.end());
    moves.insert(moves.end(), queenMoves.begin(), queenMoves.end());
    moves.insert(moves.end(), kingMoves.begin(), kingMoves.end());


    return pseudoLegalMoves;
}

std::vector<MoveData*> Eval::findLegalMoves(std::vector<MoveData*> moveList) {
    std::vector<MoveData*> legalMoves;
    std::vector<MoveData*>::iterator it;
    for (it = moveList.begin(); it != moveList.end(); ++it) {
        MoveData* move = *it;
        doMove(move);
        bool checksSelf = !checksAreValid();
        if (!checksSelf) {
            this->calculateMoveOrderScore(move);
            legalMoves.push_back(move);
        }
        undoMove(move);
    }
    std::sort(legalMoves.begin(), legalMoves.end(), compareByScore);
    return legalMoves;
}

std::vector<MoveData*> Eval::findKingMoves(uint square) {
    std::vector<MoveData*> kingMoves;
    bool turn = board->currentTurn;
    int piece = turn ? 2 : 9;
    u64 movesBitboard = kingMovesTable[square] & ~(board->pieceLocations[turn ? 1 : 8]);
    
    while (movesBitboard > 0) {
        int newSquare = BitOps::countTrailingZeroes(movesBitboard);
        int start = turn ? 10 : 3;
        int end = turn ? 15 : 8;
        int cPiece = 0;
        for (int i = start; i < end; i++) {
            if (board->pieceLocations[i] & (1ULL << newSquare)) { cPiece = i; };
        }

        MoveData* move = new MoveData(square, newSquare);
        move->piece = piece;
        move->cPiece = cPiece;
        movesBitboard ^= (1ULL << newSquare);

        kingMoves.push_back(move);
    }

    uint relevantCastle = 1;
    relevantCastle <<= (2 * !turn);
    u64 longCastleMask = 0b00001110;
    u64 shortCastleMask = 0b01100000;
    if (!board->currentTurn) { longCastleMask <<= 56; shortCastleMask <<= 56; };
    if ((board->castlingRights & (relevantCastle << 1))  & !(longCastleMask & board->pieceLocations[0])) {
        MoveData* castleMove = new MoveData(square, turn ? 2 : 58);
        castleMove->piece = piece;
        castleMove->cPiece = 0;
        kingMoves.push_back(castleMove);
    };
    if ((board->castlingRights & (relevantCastle)) & !(shortCastleMask & board->pieceLocations[0])) {
        MoveData* castleMove = new MoveData(square, turn ? 6 : 62);
        castleMove->piece = piece;
        castleMove->cPiece = 0;
        kingMoves.push_back(castleMove);
    };
    return kingMoves;
}

std::vector<MoveData*> Eval::findPawnMoves(u64 bitboard) {
    std::vector<MoveData*> pawnMoves;
    bool turn = board->currentTurn; 
    int piece = turn ? 3 : 10;
    u64 opponentPieces = board->currentTurn ? board->pieceLocations[8] : board->pieceLocations[1];
    while (bitboard != 0) {
        int oldSquare = BitOps::countTrailingZeroes(bitboard);
        int newSquare;
        bool firstMove = turn ? (oldSquare >= 8 && oldSquare < 16) : (oldSquare >= 48 && oldSquare < 56);
        u64 enPassantBitboard = board->enPassantFiles << (board->currentTurn ? 40 : 16);

        if (!((turn ? newSquare = (oldSquare + 8) : newSquare = (oldSquare - 8)) & opponentPieces)) {
            MoveData* move = new MoveData(oldSquare, newSquare);
            move->piece;
            move->cPiece;
            std::vector<MoveData*> promoMoves = addPawnMove(move);
            pawnMoves.insert(pawnMoves.end(), promoMoves.begin(), promoMoves.end());
            if (!(1ULL << (turn ? newSquare = (oldSquare + 16) : newSquare = (oldSquare - 16)) & opponentPieces)) {
                MoveData* doubleMove = new MoveData(oldSquare, newSquare);
                doubleMove->piece = piece;
                doubleMove->cPiece = 0;
                pawnMoves.push_back(doubleMove);
            }
        };
        if (oldSquare % 8 != 0 &&
            1ULL << (turn ? newSquare = (oldSquare + 7) : newSquare = (oldSquare - 9)) & (opponentPieces | enPassantBitboard)
        ) {
            int start = turn ? 10 : 3;
            int end = turn ? 15 : 8;
            int cPiece = 0;
            for (int i = start ; i < end; i++) {
                if ((1ULL << newSquare) & enPassantBitboard) {cPiece = start; };
                if (board->pieceLocations[i] & (1ULL << newSquare)) { cPiece = i; };
            }
            MoveData* move = new MoveData(oldSquare, newSquare);
            move->piece = piece;
            move->cPiece = cPiece;            
            pawnMoves.insert(pawnMoves.end(), addPawnMove(move).begin(), addPawnMove(move).begin());
        }
        if (oldSquare % 8 != 7 &&
            1ULL << (turn ? newSquare = (oldSquare + 9) : newSquare = (oldSquare - 7)) & (opponentPieces | enPassantBitboard)
        ) {
            int start = turn ? 10 : 3;
            int end = turn ? 15 : 8;
            int cPiece = 0;
            for (int i = start ; i < end; i++) {
                if ((1ULL << newSquare) & enPassantBitboard) {cPiece = start; };
                if (board->pieceLocations[i] & (1ULL << newSquare)) { cPiece = i; };
            }
            MoveData* move = new MoveData(oldSquare, newSquare);
            move->piece = piece;
            move->cPiece = cPiece;
            pawnMoves.insert(pawnMoves.end(), addPawnMove(move).begin(), addPawnMove(move).begin());
        }
        bitboard ^= 1ULL << oldSquare;
    }
    return pawnMoves;
}

std::vector<MoveData*>Eval::findBishopMoves(u64 bitboard, bool isQueen) {
    std::vector<MoveData*>bishopMoves;
    int square;
    bool turn = board->currentTurn;
    int piece = turn ? 4 : 11;
    int queen = turn ? 7 : 14;
    piece = isQueen ? queen : piece;
    while (bitboard > 0) {
        square = BitOps::countTrailingZeroes(bitboard);
        u64 mask = diagonalMasks[square];
        u64 relevantSquares = board->pieceLocations[0] & mask;
        u64 magicNumber = bishopMagics[square];
        uint shiftIndex = 64 - relevantBitsBishop[square];
        u64 movesBitboard = bishopAttacks[square][(relevantSquares * magicNumber) >> shiftIndex];
        while (movesBitboard > 0) {
            int newSquare = BitOps::countTrailingZeroes(movesBitboard);
            int start = turn ? 10 : 3;
            int end = turn ? 15 : 8;
            int cPiece = 0;
            for (int i = start ; i < end; i++) {
                if (board->pieceLocations[i] & (1ULL << newSquare)) { cPiece = i; };
            }
            MoveData* move = new MoveData(square, newSquare);
            move->piece = piece;
            move->cPiece = cPiece;
            bishopMoves.push_back(move);
            movesBitboard ^= (1ULL << newSquare);
        }
        bitboard ^= 1ULL << square;
    };
    return bishopMoves;
}

std::vector<MoveData*>Eval::findKnightMoves(u64 bitboard) {
    std::vector<MoveData*>knightMoves;
    bool turn = board->currentTurn;
    int square;
    int piece = turn ? 5 : 12;
    while (bitboard > 0) {
        square = BitOps::countTrailingZeroes(bitboard);
        u64 movesBitboard = knightMovesTable[square] & ~(board->pieceLocations[turn ? 1 : 8]);
        while (movesBitboard > 0) {
            int newSquare = BitOps::countTrailingZeroes(movesBitboard);
            int start = turn ? 10 : 3;
            int end = turn ? 15 : 8;
            int cPiece = 0;
            for (int i = start ; i < end; i++) {
                if (board->pieceLocations[i] & (1ULL << newSquare)) { cPiece = i; };
            }
            MoveData* move = new MoveData(square, newSquare);
            move->piece = piece;
            move->cPiece = cPiece;
            knightMoves.push_back(move);
            movesBitboard ^= (1ULL << newSquare);
        }
        bitboard ^= 1ULL << square;
    };
    return knightMoves;
}

std::vector<MoveData*>Eval::findRookMoves(u64 bitboard, bool isQueen) {
    std::vector<MoveData*>rookMoves;
    int square;
    bool turn = board->currentTurn;
    int piece = turn ? 6 : 13;
    int queen = turn ? 7 : 14;
    piece = isQueen ? queen : piece;
    while (bitboard > 0) {
        square = BitOps::countTrailingZeroes(bitboard);
        u64 mask = cardinalMasks[square];
        u64 relevantSquares = board->pieceLocations[0] & mask;
        u64 magicNumber = rookMagics[square];
        uint shiftIndex = 64 - relevantBitsRook[square];
        u64 movesBitboard = rookAttacks[square][(relevantSquares * magicNumber) >> shiftIndex];
        while (movesBitboard > 0) {
            int newSquare = BitOps::countTrailingZeroes(movesBitboard);
            int start = turn ? 10 : 3;
            int end = turn ? 15 : 8;
            int cPiece = 0;
            for (int i = start ; i < end; i++) {
                if (board->pieceLocations[i] & (1ULL << newSquare)) { cPiece = i; };
            }
            MoveData* move = new MoveData(square, newSquare);
            move->piece = piece;
            move->cPiece = cPiece;
            rookMoves.push_back(move);
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
            MoveData* move = *it;
            doMove(move);
            eval = std::max(eval, evalAlphaBeta((depth - 1), alpha, beta));
            if (eval >= beta) {
                killers[currentDepth].addNewKiller(move);
                storeMove = move;
                tpNodeType = BETA;
                undoMove(move);
                break;
            }
            else if (eval > alpha) {
                alpha = eval;
                storeMove = move;
                tpNodeType = EXACT;
            }
            else {
                storeMove = move;
                tpNodeType = ALPHA;
            }
            undoMove(move);
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
            MoveData* move = *it;
            doMove(move);
            eval = std::min(eval, evalAlphaBeta(depth - 1, alpha, beta));
            if (eval <= alpha) {
                killers[currentDepth].addNewKiller(move);
                storeMove = move;
                tpNodeType = ALPHA;
                undoMove(move);
                break;
            }
            else if (eval < beta) {
                beta = eval;
                storeMove = move;
                tpNodeType = EXACT;
            }
            else {
                storeMove = move;
                tpNodeType = BETA;
            }
            undoMove(move);
        }
        Transposition tp;
        tp.init(board->zobristHash, storeMove, depth, eval, tpNodeType);
        addTransposition(tp);
        return eval;
    }
        
};

void Eval::doMove(MoveData* move) {
    bool turn = board->currentTurn;
    u64* allBb = &board->pieceLocations[0];
    u64* friendlyBb = turn ? &board->pieceLocations[1] : &board->pieceLocations[8];
    u64* enemyBb = turn ? &board->pieceLocations[8] : &board->pieceLocations[1];
    u64* pieceBb =  &board->pieceLocations[move->piece];
    u64* cPieceBb = pieceBb;
    u64* pPieceBb = pieceBb;

    int oldSq = move->oldSquare;
    int newSq = move->newSquare;
    u64 oldSqBb = 1ULL << oldSq;
    u64 newSqBb = 1ULL << newSq;

    u64* hash = &board->zobristHash;
    u64 oldZobristPiece = board->zobristPseudoRandoms[move->piece - (2 + (move->piece > 7)) + (oldSq * 12)];
    u64 newZobristPiece = board->zobristPseudoRandoms[move->piece - (2 + (move->piece > 7)) + (newSq * 12)];
    u64 cZobristPiece = board->zobristPseudoRandoms[move->cPiece - (2 + (move->cPiece > 7)) + (newSq * 12)];
    u64 pZobristPiece = board->zobristPseudoRandoms[move->pPiece - (2 + (move->pPiece > 7)) + (newSq * 12)];

    if (move->cPiece != -1) { cPieceBb = &board->pieceLocations[move->cPiece]; }
    if (move->pPiece != -1) { cPieceBb = &board->pieceLocations[move->cPiece]; }

    *pieceBb ^= oldSqBb;
    *friendlyBb ^= oldSqBb;
    *allBb ^= oldSqBb;
    *hash ^= oldZobristPiece;
    if (move->pPiece != -1) { 
        *pPieceBb ^= newSqBb;
        *friendlyBb ^= oldSqBb;
        *allBb ^= newSqBb;
        *hash ^= pZobristPiece; }
    else { 
        *pieceBb ^= newSqBb; 
        *friendlyBb ^= newSqBb;
        *allBb ^= newSqBb;
        *hash ^= newZobristPiece;
    }
    if (move->cPiece != -1) { 
        *cPieceBb ^= newSqBb; 
        *enemyBb ^= newSqBb;
        *hash ^= cZobristPiece; };

    // castles 
    if (
        move->piece == 2 || move->piece == 9
    ) 
    {
        if (abs(oldSq - newSq) == 2) {
            int intSq = (move->oldSquare + move->newSquare) >> 1;
            MoveData* intMove = new MoveData(oldSq, intSq);
            doMove(intMove);
            bool intMoveValid = checksAreValid();
            undoMove(intMove);
            if (intMoveValid) {
                uint rook = turn ? 6 : 13;
                u64* rookBoard = &board->pieceLocations[rook];
                int rookSq = intSq > newSq ? oldSq - 4 : oldSq + 3;
                u64 oldZobristRook = board->zobristPseudoRandoms[rook - (2 + (move->pPiece > 7)) + (rookSq * 12)];
                u64 newZobristRook = board->zobristPseudoRandoms[rook - (2 + (move->pPiece > 7)) + (intSq * 12)];
    
                *rookBoard ^= rookSq;
                *rookBoard ^= intSq;
                *hash ^= oldZobristRook;
                *hash ^= newZobristRook;
            }
        }
        board->castlingRights &= (turn ? 0b0011 : 0b1100);
        *hash ^= board->zobristPseudoRandoms[turn ? 777 : 779];
        *hash ^= board->zobristPseudoRandoms[turn ? 778 : 780];
    }

    // Remove castling right for moved rook.
    if (move->piece == 6 || move->piece == 13) {
        int relevantCastle = 1 << ((!turn * 2) + ((oldSq % 8) == 0));
        board->castlingRights &= ~(relevantCastle);
    }

    // allow en passant
    board->enPassantFiles = 0;
    if ((move->piece == 3 || move->piece == 10)
        && abs(oldSq - newSq) == 16) {
            board->enPassantFiles = 1ULL << (oldSq % 8);
            *hash ^= board->zobristPseudoRandoms[769 + (oldSq % 8)];
        }

    enPassantHistory.push_back(board->enPassantFiles);
    castlingRightsHistory.push_back(board->castlingRights);
    
    board->currentTurn = !turn;
    *hash ^= board->zobristPseudoRandoms[768];
}

void Eval::undoMove(MoveData* move) {
    bool turn = board->currentTurn;
    board->currentTurn = !turn;
    u64* hash = &board->zobristHash;
    *hash ^= board->zobristPseudoRandoms[768];

    u64* allBb = &board->pieceLocations[0];
    u64* friendlyBb = turn ? &board->pieceLocations[1] : &board->pieceLocations[8];
    u64* enemyBb = turn ? &board->pieceLocations[8] : &board->pieceLocations[1];
    u64* pieceBb =  &board->pieceLocations[move->piece];
    u64* cPieceBb = pieceBb;
    u64* pPieceBb = pieceBb;

    int oldSq = move->oldSquare;
    int newSq = move->newSquare;
    u64 oldSqBb = 1ULL << oldSq;
    u64 newSqBb = 1ULL << newSq;

    u64 oldZobristPiece = board->zobristPseudoRandoms[move->piece - (2 + (move->piece > 7)) + (oldSq * 12)];
    u64 newZobristPiece = board->zobristPseudoRandoms[move->piece - (2 + (move->piece > 7)) + (newSq * 12)];
    u64 cZobristPiece = board->zobristPseudoRandoms[move->cPiece - (2 + (move->cPiece > 7)) + (newSq * 12)];
    u64 pZobristPiece = board->zobristPseudoRandoms[move->pPiece - (2 + (move->pPiece > 7)) + (newSq * 12)];

    if (move->cPiece != -1) { cPieceBb = &board->pieceLocations[move->cPiece]; }
    if (move->pPiece != -1) { cPieceBb = &board->pieceLocations[move->cPiece]; }

    *pieceBb ^= oldSqBb;
    *friendlyBb ^= oldSqBb;
    *allBb ^= oldSqBb;
    *hash ^= oldZobristPiece;
    if (move->pPiece != -1) { 
        *pPieceBb ^= newSqBb;
        *friendlyBb ^= oldSqBb;
        *allBb ^= newSqBb;
        *hash ^= pZobristPiece; }
    else { 
        *pieceBb ^= newSqBb; 
        *friendlyBb ^= newSqBb;
        *allBb ^= newSqBb;
        *hash ^= newZobristPiece;
    }
    if (move->cPiece != -1) { 
        *cPieceBb ^= newSqBb; 
        *enemyBb ^= newSqBb;
        *hash ^= cZobristPiece; };
    
    // castling 
    if (
        move->piece == 2 || move->piece == 9
    ) 
    {
        if (abs(oldSq - newSq) == 2) {
            int intSq = (move->oldSquare + move->newSquare) >> 1;
            MoveData* intMove = new MoveData(oldSq, newSq);
            
            uint rook = turn ? 6 : 13;
            u64* rookBoard = &board->pieceLocations[rook];
            int rookSq = intSq > newSq ? oldSq - 4 : oldSq + 3;
            u64 oldZobristRook = board->zobristPseudoRandoms[rook - (2 + (move->pPiece > 7)) + (rookSq * 12)];
            u64 newZobristRook = board->zobristPseudoRandoms[rook - (2 + (move->pPiece > 7)) + (intSq * 12)];

            *rookBoard ^= rookSq;
            *rookBoard ^= intSq;
            *hash ^= oldZobristRook;
            *hash ^= newZobristRook;
        }
    }
    revertBoardRights();
}

void Eval::revertBoardRights() {
    castlingRightsHistory.pop_back();
    enPassantHistory.pop_back();
    int lastCastlingRights = castlingRightsHistory.back();
    int lastEnpassantFiles = enPassantHistory.back();

    int castlingDifference = lastCastlingRights ^ board->castlingRights;
    int enPassantDifference = lastEnpassantFiles ^ board->enPassantFiles;

    while (castlingDifference > 0) {
        int right = BitOps::countTrailingZeroes(castlingDifference);
        board->zobristHash ^= board->zobristPseudoRandoms[777 + right];
        castlingDifference ^= 1ULL << right;
    }
    while (enPassantDifference > 0) {
        int file = BitOps::countTrailingZeroes(enPassantDifference);
        board->zobristHash ^= board->zobristPseudoRandoms[769 + file];
        enPassantDifference ^= 1ULL << file;
    }
    board->castlingRights = lastCastlingRights;
    board->enPassantFiles = lastEnpassantFiles;
};

void Eval::calculateMoveOrderScore(MoveData* moveData) {
    const int PIECEVALUES[7] = {0, 0, 100, 300, 300, 500, 900};
    float score = 0;
    // 1 refutation from TT (handled elsewhere)
    // 2 killer moves
    if (moveData == this->killers[currentDepth].first) { moveData->score = INFINITY - 1; }
    else if (moveData == killers[currentDepth].second) { moveData->score = INFINITY - 2; };
    // 3 captures in order of risk/reward
    uint capture = moveData->cPiece;
    if (capture) {
        if (capture >= 6) {
            score += capture;
        }
        else {            
            score += PIECEVALUES[capture+1] * 2;
        }
    };
    moveData->score = score;
};

std::vector<MoveData*> Eval::addPawnMove(MoveData* move) {
    std::vector<MoveData*> moves;
    bool turn = board->currentTurn;
    int start = -1;
    int stop = 0;
    if ((move->newSquare > 55 && turn) || (move->newSquare < 8 && !turn)) {
        start = turn ? 4 : 11; stop = turn ? 8 : 15;
    }
    for (int i = start; i < stop; i++) {
        MoveData moveCopy = *move;
        moveCopy.pPiece = i;
        moves.push_back(&moveCopy);
    }
    return moves;
}

bool Eval::checksAreValid() {
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
