#ifndef EVALUATION_HPP
#define EVALUATION_HPP

#include "chess.hpp"

namespace chess {

class Evaluation {
public:
    // Piece values
    int pawnValue = 100;
    int knightValue = 300;
    int bishopValue = 320;
    int rookValue = 500;
    int queenValue = 900;
    
    // Game state constants
    float endgameMaterial = rookValue * 2 + bishopValue + knightValue;
    int maxDepth = 4;
    int WHITE = 0;
    int BLACK = 1;
    
    // Board state
    Board board;

    /**
     * Evaluates the current position on the board
     * @param board The chess board to evaluate
     * @return Integer score from the perspective of the side to move
     */
    int evaluate(Board board);

    /**
     * Calculates the weight of the endgame phase based on material
     * @param materialCountWithoutPawns Total material value excluding pawns
     * @return Float between 0 and 1 representing endgame phase
     */
    float endgamePhaseWeight(int materialCountWithoutPawns);

    /**
     * Evaluates king safety and piece coordination in endgame positions
     * @param friendlyIndex Index of friendly color
     * @param opponentIndex Index of opponent color
     * @param friendlyMaterial Total material value of friendly pieces
     * @param opponentMaterial Total material value of opponent pieces
     * @param endgameWeight Current weight of endgame phase
     * @return Integer score for mop-up evaluation
     */
    int mopUpEval(int friendlyIndex, int opponentIndex, int friendlyMaterial, 
                  int opponentMaterial, float endgameWeight);

    /**
     * Counts total material value for given color
     * @param color Color to evaluate (WHITE or BLACK)
     * @return Total material value
     */
    int countMaterial(int color);

    /**
     * Evaluates piece placement using piece-square tables
     * @param color Color to evaluate
     * @param endgamePhaseWeight Current weight of endgame phase
     * @return Integer score for piece placement
     */
    int evaluatePieceSquareTables(int color, float endgamePhaseWeight);

    /**
     * Evaluates specific piece type placement using piece-square table
     * @param table Piece-square table to use
     * @param pieces Bitboard of pieces to evaluate
     * @param isWhite Whether evaluating for white pieces
     * @return Integer score for piece placement
     */
    int evaluatePieceSquareTable(const int* table, Bitboard pieces, bool isWhite);
};

} // namespace chess

#endif // EVALUATION_HPP
