#include "evaluation.hpp"
#include "tables.hpp"
#include "precompute.hpp"

namespace chess {

int Evaluation::evaluate(Board board) {
    this->board = board;
    int whiteEval = 0;
    int blackEval = 0;

    int whiteMaterial = countMaterial(WHITE);
    int blackMaterial = countMaterial(BLACK);

    int whiteMaterialWithoutPawns = whiteMaterial - board.pieces(PieceType::PAWN, Color::WHITE).count() * pawnValue;
    int blackMaterialWithoutPawns = blackMaterial - board.pieces(PieceType::PAWN, Color::BLACK).count() * pawnValue;
    float whiteEndgamePhaseWeight = endgamePhaseWeight(whiteMaterialWithoutPawns);
    float blackEndgamePhaseWeight = endgamePhaseWeight(blackMaterialWithoutPawns);

    whiteEval += whiteMaterial;
    blackEval += blackMaterial;
   	whiteEval += mopUpEval(WHITE, BLACK, whiteMaterial, blackMaterial, blackEndgamePhaseWeight);
    blackEval += mopUpEval(BLACK, WHITE, blackMaterial, whiteMaterial, whiteEndgamePhaseWeight);

    whiteEval += evaluatePieceSquareTables(WHITE, blackEndgamePhaseWeight);
    blackEval += evaluatePieceSquareTables(BLACK, whiteEndgamePhaseWeight);

    int eval = whiteEval - blackEval;
	
    int perspective = static_cast<int>(board.sideToMove().internal()) == WHITE ? 1 : -1;
    return eval * perspective;
}

float Evaluation::endgamePhaseWeight(int materialCountWithoutPawns) {
    return 1 - fmin(1, materialCountWithoutPawns / endgameMaterial);
}

int Evaluation::mopUpEval(int friendlyIndex, int opponentIndex, int friendlyMaterial, int opponentMaterial, float endgameWeight) {
    int mopUpScore = 0;
    if (friendlyMaterial > opponentMaterial + pawnValue * 2 && endgameWeight > 0) {
        int friendlyKingSquare = board.kingSq(friendlyIndex).index();
        int opponentKingSquare = board.kingSq(opponentIndex).index();
        mopUpScore += PrecomputedMoveData::centreManhattanDistance[opponentKingSquare] * 10;
        mopUpScore += (14 - PrecomputedMoveData::numRookMovesToReachSquare(friendlyKingSquare, opponentKingSquare)) * 4;
        
        return (int)(mopUpScore * endgameWeight);
    }
    return 0;
}

int Evaluation::countMaterial(int color) {
    int material = 0;
    material += board.pieces(PieceType::PAWN, color).count() * pawnValue;
    material += board.pieces(PieceType::KNIGHT, color).count() * knightValue;
    material += board.pieces(PieceType::BISHOP, color).count() * bishopValue;
    material += board.pieces(PieceType::ROOK, color).count() * rookValue;
    material += board.pieces(PieceType::QUEEN, color).count() * queenValue;
    return material;
}

int Evaluation::evaluatePieceSquareTables(int color, float endgamePhaseWeight) {
    int value = 0;
    bool isWhite = color == WHITE;
    value += evaluatePieceSquareTable(PieceSquareTable::pawns, board.pieces(PieceType::PAWN, color), isWhite);
    value += evaluatePieceSquareTable(PieceSquareTable::knights, board.pieces(PieceType::KNIGHT, color), isWhite);
    value += evaluatePieceSquareTable(PieceSquareTable::bishops, board.pieces(PieceType::BISHOP, color), isWhite);
    value += evaluatePieceSquareTable(PieceSquareTable::rooks, board.pieces(PieceType::ROOK, color), isWhite);
    value += evaluatePieceSquareTable(PieceSquareTable::queens, board.pieces(PieceType::QUEEN, color), isWhite);
    int kingEarlyPhase = PieceSquareTable::read(PieceSquareTable::kingMiddle, board.kingSq(color).index(), isWhite);
    value += (int)(kingEarlyPhase * (1 - endgamePhaseWeight));
    return value;
}

int Evaluation::evaluatePieceSquareTable(const int* table, Bitboard pieces, bool isWhite) {
    int value = 0;
    std::vector<int> indices;
    while (!pieces.empty()) {
        indices.push_back(pieces.pop());
    }
    for (int i = 0; i < indices.size(); i++) {
        value += PieceSquareTable::read(table, indices[i], isWhite);
    }
    return value;
}

} // namespace chess
