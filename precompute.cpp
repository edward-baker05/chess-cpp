#include "precompute.hpp"
#include <algorithm>
#include <cmath>

namespace chess {

std::array<std::array<int, 8>, 64> PrecomputedMoveData::numSquaresToEdge;
std::array<std::vector<uint8_t>, 64> PrecomputedMoveData::knightMoves;
std::array<std::vector<uint8_t>, 64> PrecomputedMoveData::kingMoves;
std::array<std::vector<int>, 64> PrecomputedMoveData::pawnAttacksWhite;
std::array<std::vector<int>, 64> PrecomputedMoveData::pawnAttacksBlack;
std::array<int, 127> PrecomputedMoveData::directionLookup;
std::array<uint64_t, 64> PrecomputedMoveData::kingAttackBitboards;
std::array<uint64_t, 64> PrecomputedMoveData::knightAttackBitboards;
std::array<std::array<uint64_t, 2>, 64> PrecomputedMoveData::pawnAttackBitboards;
std::array<uint64_t, 64> PrecomputedMoveData::rookMoves;
std::array<uint64_t, 64> PrecomputedMoveData::bishopMoves;
std::array<uint64_t, 64> PrecomputedMoveData::queenMoves;
std::array<std::array<int, 64>, 64> PrecomputedMoveData::orthogonalDistance;
std::array<std::array<int, 64>, 64> PrecomputedMoveData::kingDistance;
std::array<int, 64> PrecomputedMoveData::centreManhattanDistance;

void PrecomputedMoveData::initialize() {
    for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
        int y = squareIndex / 8;
        int x = squareIndex % 8;
        int north = 7 - y;
        int south = y;
        int west = x;
        int east = 7 - x;
        numSquaresToEdge[squareIndex] = { north, south, west, east, 
                                        std::min(north, west), 
                                        std::min(south, east), 
                                        std::min(north, east), 
                                        std::min(south, west) };
        if (x > 0) {
            if (y < 7) pawnAttacksWhite[squareIndex].push_back(squareIndex + 7);
            if (y > 0) pawnAttacksBlack[squareIndex].push_back(squareIndex - 9);
        }
        if (x < 7) {
            if (y < 7) pawnAttacksWhite[squareIndex].push_back(squareIndex + 9);
            if (y > 0) pawnAttacksBlack[squareIndex].push_back(squareIndex - 7);
        }

        uint64_t knightBitboard = 0;
        static const int allKnightJumps[] = { 15, 17, -17, -15, 10, -6, 6, -10 };
        for (int jump : allKnightJumps) {
            int targetSquare = squareIndex + jump;
            if (targetSquare >= 0 && targetSquare < 64) {
                int targetY = targetSquare / 8;
                int targetX = targetSquare % 8;
                if (std::max(std::abs(x - targetX), std::abs(y - targetY)) == 2) {
                    knightMoves[squareIndex].push_back(targetSquare);
                    knightBitboard |= 1ULL << targetSquare;
                }
            }
        }
        knightAttackBitboards[squareIndex] = knightBitboard;
    }

    for (int squareA = 0; squareA < 64; squareA++) {
        int rankA = squareA / 8;
        int fileA = squareA % 8;
        int fileDistFromCenter = std::max(3 - fileA, fileA - 4);
        int rankDistFromCenter = std::max(3 - rankA, rankA - 4);
        centreManhattanDistance[squareA] = fileDistFromCenter + rankDistFromCenter;

        for (int squareB = 0; squareB < 64; squareB++) {
            int rankB = squareB / 8;
            int fileB = squareB % 8;
            int rankDist = std::abs(rankA - rankB);
            int fileDist = std::abs(fileA - fileB);
            orthogonalDistance[squareA][squareB] = fileDist + rankDist;
            kingDistance[squareA][squareB] = std::max(fileDist, rankDist);
        }
    }
}

int PrecomputedMoveData::numRookMovesToReachSquare(int startSquare, int targetSquare) {
    return orthogonalDistance[startSquare][targetSquare];
}

int PrecomputedMoveData::numKingMovesToReachSquare(int startSquare, int targetSquare) {
    return kingDistance[startSquare][targetSquare];
}

} // namespace chess
