#include "engine.hpp"
using namespace chess;
using namespace std;

Engine::Engine(int maxDepth, Board board) 
    : maxDepth_(maxDepth), 
      board_(board), 
      transposition_(256000), 
      search_(board, AISettings{maxDepth}) 
{
    search_.settings.useIterativeDeepening = true;
    search_.settings.depth = maxDepth; 
}

void Engine::setPosition(Board board) {
    board_ = board;
}

chess::Move Engine::getMove(Board board) {
    board_ = board;
    team_ = board_.sideToMove();
    cout << "Maximising score for " << team_ << endl;

    cout << "Starting search..." << endl;
    search_.startSearch(board_); 
    auto [bestMove, bestEval] = search_.getSearchResult(); 

    cout << "Best move: " << chess::uci::moveToSan(board_, bestMove) 
              << " Eval: " << bestEval << endl;

    return bestMove; 
}
