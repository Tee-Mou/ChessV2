#include "../inc/Board.h"

void Board::generateZobristPsuedoRandoms(u64 seed) { 
// Values for a from Steele GL., Vigna S. 'Computationally easy, spectrally good multipliers for congruential pseudorandom number generators', 2022.
// Zobrist randoms are generated for each square/piece combination, from a1-h8, with each square having 12 assigned numbers. These are in order of ascending
// piece capture value (King = 0 => Queen = 9), with white pieces first, then black pieces. These map array addresses [0...767]. The numbers after this indicate, 
// in order, whether it is white's turn [768], which of the 8 files contain En Passant (skipped) squares [769...766], and the castling rights of each colour, in the order 
// white- short, long, black- short, long [777...780].
//  
    for (int i = 0; i < sizeof(this->zobristPseudoRandoms); i++) {
        uint m = 1ULL << 32;
        uint a = 0x93d765dd;
        seed = (seed * a) % m;
        this->zobristPseudoRandoms[i] = seed;
    }
}

u64 Board::calculateZobristHash(Board* board) {
    u64 hash;
    for (int i = 2; i < 15; i++) {
        if (i == 8) { continue; };
        u64 bitboard = board->pieceLocations[i];
        std::vector<uint> squares;
        while (bitboard > 0) {
            squares.push_back(BitOps::countTrailingZeroes(bitboard));
            bitboard &= ~(1ULL << squares[sizeof(squares) - 1]);
        };
        std::vector<uint>::iterator it;
        for(it = squares.begin(); it != squares.end(); ++it) {
            uint square = *it;
            uint piece = i - 2;
            piece = i > 8 ? piece : piece - 1;
            u64 zobristNumber = this->zobristPseudoRandoms[(12 * square) + piece];
            hash^=zobristNumber;
        };
    };
    if (board->currentTurn) { hash^=this->zobristPseudoRandoms[768]; };
    for (int i = 0; i < 8; i++) {
        if (board->enPassantFiles & (1ULL << i)) { hash^=this->zobristPseudoRandoms[769 + i]; }
    };
    if (board->whiteCastles.shortCastle) { hash^=this->zobristPseudoRandoms[777]; };
    if (board->whiteCastles.longCastle) { hash^=this->zobristPseudoRandoms[778]; };
    if (board->blackCastles.shortCastle) { hash^=this->zobristPseudoRandoms[779]; };
    if (board->blackCastles.longCastle) { hash^=this->zobristPseudoRandoms[780]; };
    return hash;
}