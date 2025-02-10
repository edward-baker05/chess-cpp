#include "engine.hpp"
using namespace chess;
using namespace std;

Engine::Engine(int maxDepth, Board board) 
    : maxDepth_(maxDepth), 
      board_(board), 
      transposition_(board, 256000), 
      search_(board, AISettings{maxDepth}) // Initialize the Search object
{
    // Configure AISettings for the Search class
    search_.settings.useIterativeDeepening = true; // Example setting
    search_.settings.depth = maxDepth; // Set the search depth
}

chess::Move Engine::getMove(Board& board) {
    board_ = board;
    team_ = board_.sideToMove();
    cout << "Maximising score for " << team_ << endl;

    // Use the Search class to find the best move
    cout << "Starting search..." << endl;
    search_.startSearch(board); // Start the search
    auto [bestMove, bestEval] = search_.getSearchResult(); // Retrieve the best move and evaluation

    // Log the best move and evaluation
    cout << "Best move: " << chess::uci::moveToSan(board_, bestMove) 
              << " Eval: " << bestEval << endl;

    return bestMove; // Return the best move
}
