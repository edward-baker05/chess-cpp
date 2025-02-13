#include <vector>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include "chess.hpp"
#include "evaluation.hpp"
#include "transposition.hpp"

namespace chess {

struct AISettings {
    int depth;
    bool useTranspositionTable = true;
    bool clearTTEachMove = true;
    bool useIterativeDeepening = true;
    bool useFixedDepthSearch = false;
    bool endlessSearchMode = true;
    bool useThreading = false;

    AISettings(int depth) {
        this->depth = depth;
    }
};

class Search {
public:
    struct SearchDiagnostics {
        int lastCompletedDepth = 0;
        std::string moveVal;
        std::string move;
        int eval = 0;
        bool isBook = false;
        int numPositionsEvaluated = 0;
    };
    AISettings settings;

    Search(Board& board, AISettings settings);
    ~Search() = default;

    Search(const Search&) = delete;
    Search& operator=(const Search&) = delete;

    void startSearch(Board board);
    void endSearch();
    std::pair<Move, int> getSearchResult() const;
    
    std::function<void(Move)> onSearchComplete;
    
    const SearchDiagnostics& getDiagnostics() const { return searchDiagnostics; }

    static bool isMateScore(int score);
    static int numPlyToMateFromScore(int score);

private:
    static constexpr int transpositionTableSize = 64000;
    static constexpr int immediateMateScore = 100000;
    static constexpr int positiveInfinity = 9999999;
    static constexpr int negativeInfinity = -positiveInfinity;
    std::string currentRootFEN;

    int searchMoves(int depth, int plyFromRoot, int alpha, int beta, Board& newBoard);
    void parallelSearch(int depth);
    int quiescenceSearch(int alpha, int beta, Board& currentBoard);
    void initDebugInfo();
    void logDebugInfo();
    void announceMate();
    int evaluatePosition();
    void orderMoves(Movelist& moves, Board& currentBoard);
    int capturedPieceValue(Board& board, Move move);

    Board board;
    TranspositionTable transpositionTable;
    Move bestMoveThisIteration;
    int bestEvalThisIteration;
    Move bestMove;
    int bestEval;
    int currentIterativeSearchDepth;
    bool abortSearch;
    Evaluation evaluation;

    SearchDiagnostics searchDiagnostics;
    int numNodes;
    int numQNodes;
    int numCutoffs;
    int numTranspositions;
    std::chrono::steady_clock::time_point searchStartTime;
};

}
