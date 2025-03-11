#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <vector>
#include <random>
#include <limits>
#include <iostream>
#include "chess.hpp"
#include "search.hpp"
#include "transposition.hpp"

namespace chess {

class Engine {
public:
    Engine(int maxDepth, Board board);
    void setPosition(Board board);
    Move getMove(Board board);

private:
    int maxDepth_; // Maximum search depth
    Board board_; // Current board state
    Color team_; // The side the engine is playing as
    TranspositionTable transposition_; // Transposition table for caching evaluations
    Search search_; // Search object for finding the best move

    // Old search method (to be removed or replaced)
    int search(Board& board, int depth, int alpha, int beta);

    // Evaluation counter (if needed)
    int evaluated_ = 0;
};

} // namespace chess

#endif // ENGINE_HPP
