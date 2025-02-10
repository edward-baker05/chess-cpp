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

    // Disable copying
    Search(const Search&) = delete;
    Search& operator=(const Search&) = delete;

    void startSearch(Board board);
    void endSearch();
    std::pair<Move, int> getSearchResult() const;
    
    // Callback for when search completes
    std::function<void(Move)> onSearchComplete;
    
    // Public access to diagnostics
    const SearchDiagnostics& getDiagnostics() const { return searchDiagnostics; }

    static bool isMateScore(int score);
    static int numPlyToMateFromScore(int score);

private:
    static constexpr int transpositionTableSize = 64000;
    static constexpr int immediateMateScore = 100000;
    static constexpr int positiveInfinity = 9999999;
    static constexpr int negativeInfinity = -positiveInfinity;

    int searchMoves(int depth, int plyFromRoot, int alpha, int beta);
    int quiescenceSearch(int alpha, int beta);
    void initDebugInfo();
    void logDebugInfo();
    void announceMate();
    int evaluatePosition();
    void orderMoves(Movelist moves);

    // Member variables
    Board board;
    TranspositionTable transpositionTable;
    Move bestMoveThisIteration;
    int bestEvalThisIteration;
    Move bestMove;
    int bestEval;
    int currentIterativeSearchDepth;
    bool abortSearch;
    Evaluation evaluation;

    // Diagnostics
    SearchDiagnostics searchDiagnostics;
    int numNodes;
    int numQNodes;
    int numCutoffs;
    int numTranspositions;
    std::chrono::steady_clock::time_point searchStartTime;
};

} // namespace Chess
