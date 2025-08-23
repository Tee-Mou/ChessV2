class Game{
    public:
        Game();
        ~Game();

        long long generatePseudoLegalMoves();
        long long generateLegalMoves();
        long long generateAttackedSquares();
        long long generateAttacksThisSquare();
        bool isCheck();

        //==== bitboards ====//
        long long pieceLocations[15];
        long long pawnMasks[2]; // Bit Masks for Pawns
        long long edgeMasks[4]; // Bit Masks for Kings
        long long cardinalMasks[64]; // Bit Masks for Rooks
        long long diagonalMasks[64]; // Bit Masks for Bishops
        
    
        bool currentTurn = 1;
};