#ifndef PRECOMPUTED_MOVE_DATA_HPP
#define PRECOMPUTED_MOVE_DATA_HPP

#include <array>
#include <vector>
#include <cstdint>
#include "chess.hpp"

namespace chess {

class PrecomputedMoveData {
public:
    static void initialize();
    static int numRookMovesToReachSquare(int startSquare, int targetSquare);
    static int numKingMovesToReachSquare(int startSquare, int targetSquare);

    static std::array<std::array<int, 8>, 64> numSquaresToEdge;
    static std::array<std::vector<uint8_t>, 64> knightMoves;
    static std::array<std::vector<uint8_t>, 64> kingMoves;
    static std::array<std::vector<int>, 64> pawnAttacksWhite;
    static std::array<std::vector<int>, 64> pawnAttacksBlack;
    static std::array<int, 127> directionLookup;
    static std::array<uint64_t, 64> kingAttackBitboards;
    static std::array<uint64_t, 64> knightAttackBitboards;
    static std::array<std::array<uint64_t, 2>, 64> pawnAttackBitboards;
    static std::array<uint64_t, 64> rookMoves;
    static std::array<uint64_t, 64> bishopMoves;
    static std::array<uint64_t, 64> queenMoves;
    static std::array<std::array<int, 64>, 64> orthogonalDistance;
    static std::array<std::array<int, 64>, 64> kingDistance;
    static std::array<int, 64> centreManhattanDistance;
};

} // namespace chess

#endif // PRECOMPUTED_MOVE_DATA_HPP
