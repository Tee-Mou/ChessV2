#include <iostream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include "Board.h"
using u64 = uint64_t;
using uint = unsigned int;

static class MoveGen{
    public:

        enum MagicPiece{ROOK, BISHOP};
        struct MoveData {
            uint move;
            uint score; // Move score is used to order the evaluation of moves.
            bool check;
            uint piece;
            uint eval; // The board evaluation if this move is played.
        };

        MoveGen() {
            // Creating initial position
            this->setBitboards();

            // Initialise lookup tables for piece moves.
            // this->initMagicLookupTable();
            this->initKingLookupTable();
            this->initKnightLookupTable();
            this->initSliderAttacksLookupTable(BISHOP);
            this->initSliderAttacksLookupTable(ROOK);

            // Initialise Zobrist random numbers using LCG.
            this->generateZobristPsuedoRandoms(8752137612383702536ULL);
        };
        ~MoveGen();

        void setBitboards();
        static u64 initBlockersPermutation(uint index, uint relevantBits, u64 mask);

        // gamestate moves
        static std::vector<uint> findKingMoves(Board* board, uint square);
        static std::vector<uint> findPawnMoves(Board* board, u64 bitboard);
        static std::vector<uint> findBishopMoves(Board* board, u64 bitboard);
        static std::vector<uint> findKnightMoves(u64 bitboard);
        static std::vector<uint> findRookMoves(Board* board, u64 bitboard);
        static std::vector<uint> findPseudoLegalMoves(Board* board);
        static std::vector<MoveData> findLegalMoves(Board* board, std::vector<uint> plm);
        static u64 findAttackedSquares();
        static u64 findAttacksThisSquare(uint square);

        // verify legal moves
        static Board* doMove(Board* board, uint move);
        static Board* undoMove(Board* board, uint move);
        static bool checksAreValid(Board* board, uint move);

        // find magic numbers 
        static u64 initMagicAttacks(uint square, MagicPiece piece);
        static u64 initBishopAttacksForPosition(uint square, u64 blockers);
        static u64 initRookAttacksForPosition(uint square, u64 blockers);

        // init basic lookup tables
        static void initMagicLookupTable();
        static void initKingLookupTable();
        static void initKnightLookupTable();
        static void initSliderAttacksLookupTable(MagicPiece piece);

        // transposition tables
        static void generateZobristPsuedoRandoms(u64 seed);
        static u64 calculateZobristHash(Board* board);

        static Eval* evaluator;
        static u64 pawnMasks[2]; // Bit Masks for Pawns
        static u64 edgeMasks[4]; // Bit Masks for Kings
        static u64 cardinalMasks[64]; // Bit Masks for Rooks
        static u64 diagonalMasks[64]; // Bit Masks for Bishops
        static u64 kingMovesTable[64]; // Lookup Table for King Moves
        static u64 knightMovesTable[64]; // Lookup Table for Knight Moves
        static constexpr u64 bishopMagics[64] = {
            2468047380310671617,
            13585776195411976,
            4578369138066688,
            28187086540507136,
            144414530116517924,
            109249812066476032,
            900791411448742952,
            2882444502504572992,
            35326139695176,
            9227893234208899585,
            2550871304972288,
            18041972216823824,
            565157835056192,
            4611721789133817856,
            5764929698463353106,
            9223407358666345732,
            9817847213576421632,
            4504047111438449,
            2308099414448930834,
            2251843302899713,
            9228438604185927712,
            9147946541268992,
            1169247126558868018,
            5188218258624350213,
            2322529472549120,
            290408593626112,
            293903873398492160,
            18018796858020368,
            2351161582191394816,
            74945461345099776,
            633908723451905,
            4612108370512970816,
            1143766971057216,
            12396160345160294785,
            145245494619218432,
            2323892613574819984,
            37189889887969408,
            9227893263567438850,
            13515201223606545,
            72199439729099012,
            1226106133496760578,
            145205093206016,
            580983078005639168,
            4611967776947439624,
            2017902973120708864,
            9025345525121296,
            10177655219421256,
            5481445965296636424,
            167267770826752,
            9062179400057344,
            54343646015660032,
            54060927368298880,
            576470785653474852,
            7028016823513776384,
            1236255709369991296,
            9232381453470345352,
            10525477486739982464,
            4630263530134855680,
            36099174625839140,
            2350879005491724800,
            4900479894960546818,
            2344433802610960,
            432636395929076740,
            9224515668552188544,
        };
        static u64 bishopAttacks[64][512];
        static constexpr u64 rookMagics[64] = {
            612489620193542176,
            594545520093958144,
            72066682473947136,
            9835879178497426560,
            36033195199725571,
            6485194459134382088,
            252203778181169408,
            36031545936520448,
            1548122035618868,
            1747537667788112000,
            648799958758080771,
            562984615157824,
            1775262712482562192,
            144537950397400064,
            25333306249791492,
            27162336879968384,
            36031820680273920,
            141838073790497,
            576479445158731776,
            4614010386143781377,
            141287378387968,
            564053760541696,
            216194772367843984,
            74311592879129756,
            5062363196711550976,
            4580574033904384,
            288512130302353408,
            8806835683456,
            11529232659729875200,
            4935949591792189568,
            2594117795328295489,
            1297054843214516484,
            1157478155678646404,
            576601627239137281,
            16149908332478472320,
            4503633995501696,
            46161930548675712,
            36591757718193152,
            4630544859063648768,
            162200509816504388,
            468444732138225700,
            7223773974369411072,
            576495937766621200,
            2468271663515762696,
            2325546395291484164,
            563018740137992,
            9011605891252480,
            6935553461345976340,
            3468053471521966592,
            70373048590592,
            3243014081611825536,
            580964422865387648,
            1188954699806572672,
            9024809696495617,
            72339077605885184,
            141854189045248,
            72128581811470337,
            4776349154683129985,
            288265560726192385,
            4504703702933509,
            281492290798097,
            54606216419610882,
            2311753986174108705,
            2305861152296411394,
        };
        static u64 rookAttacks[64][4096];
        static constexpr uint relevantBitsBishop[64] = {
            6, 5, 5, 5, 5, 5, 5, 6,
            5, 5, 5, 5, 5, 5, 5, 5,
            5, 5, 7, 7, 7, 7, 5, 5,
            5, 5, 7, 9, 9, 7, 5, 5,
            5, 5, 7, 9, 9, 7, 5, 5,
            5, 5, 7, 7, 7, 7, 5, 5,
            5, 5, 5, 5, 5, 5, 5, 5,
            6, 5, 5, 5, 5, 5, 5, 6,
        }; // Lookup Table for Bishop Relevant Occupancy
        static constexpr uint relevantBitsRook[64] = {
            12, 11, 11, 11, 11, 11, 11, 12,
            11, 10, 10, 10, 10, 10, 10, 11,
            11, 10, 10, 10, 10, 10, 10, 11,
            11, 10, 10, 10, 10, 10, 10, 11,
            11, 10, 10, 10, 10, 10, 10, 11,
            11, 10, 10, 10, 10, 10, 10, 11,
            11, 10, 10, 10, 10, 10, 10, 11,
            12, 11, 11, 11, 11, 11, 11, 12,
        }; // Lookup Table for Rook Relevant Occupancy
        static u64 zobristPseudoRandoms[781];
};