#include "search.hpp"
#include "evaluation.hpp"
#include "transposition.hpp"
#include <cmath>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdio>

using namespace std;
namespace chess {

Search::Search(Board& board, AISettings settings) 
    : board(board)
    , settings(settings)
    , transpositionTable(board, 1 << 20)
    , numNodes(0)
    , numQNodes(0)
    , numCutoffs(0)
    , numTranspositions(0)
    , bestEval(0)
    , bestEvalThisIteration(0)
    , currentIterativeSearchDepth(0)
    , abortSearch(false)
    , evaluation(Evaluation())
{
}

void Search::startSearch(Board board) {
    initDebugInfo();
    this->board = board;

    bestEvalThisIteration = bestEval = 0;
    bestMoveThisIteration = bestMove = Move::NO_MOVE;
    currentIterativeSearchDepth = 0;
    abortSearch = false;
    searchDiagnostics = SearchDiagnostics();

    if (settings.useIterativeDeepening) {
	auto start = chrono::steady_clock::now();
	cout << "Using iterative deepening" << endl;
        int targetDepth = settings.useFixedDepthSearch ? settings.depth : numeric_limits<int>::max();

        for (int searchDepth = 1; searchDepth <= targetDepth; searchDepth++) {
	    auto end = chrono::steady_clock::now();
            int elapsed = chrono::duration<double, milli>(end - start).count();
            cout << "Depth " << searchDepth - 1 << " complete in " << elapsed << endl;

            if (searchDepth > 4) {
                break;
            }

            cout << "Searching depth " << searchDepth << endl;
            searchMoves(searchDepth, 0, negativeInfinity, positiveInfinity);

            if (abortSearch) {
                break;
            } else {
                currentIterativeSearchDepth = searchDepth;
                bestMove = bestMoveThisIteration;
                bestEval = bestEvalThisIteration;

                searchDiagnostics.lastCompletedDepth = searchDepth;
                searchDiagnostics.move = uci::moveToUci(bestMove);
                searchDiagnostics.eval = bestEval;
                searchDiagnostics.moveVal = uci::moveToUci(bestMove); 

                if (isMateScore(bestEval) && !settings.endlessSearchMode) {
                    break;
                }
            }
        }
    } else {
        searchMoves(settings.depth, 0, negativeInfinity, positiveInfinity);
        bestMove = bestMoveThisIteration;
        bestEval = bestEvalThisIteration;
    }

    if (onSearchComplete) {
        onSearchComplete(bestMove);
    }

    if (!settings.useThreading) {
        logDebugInfo();
    }
}

pair<Move, int> Search::getSearchResult() const {
    return {bestMove, bestEval};
}

void Search::endSearch() {
    abortSearch = true;
}

int Search::searchMoves(int depth, int plyFromRoot, int alpha, int beta) {
    if (abortSearch) {
        return 0;
    }

    if (plyFromRoot > 0) {
        if (board.isRepetition(1)) {
            return 0;
        }

        alpha = max(alpha, -immediateMateScore + plyFromRoot);
        beta = min(beta, immediateMateScore - plyFromRoot);
        if (alpha >= beta) {
            return alpha;
        }
    }

    int ttEval = transpositionTable.lookupEvaluation(depth, plyFromRoot, alpha, beta, board);
    if (ttEval != TranspositionTable::lookupFailed) {
        numTranspositions++;
        if (plyFromRoot == 0) {
            bestMoveThisIteration = transpositionTable.getStoredMove(board);
            bestEvalThisIteration = transpositionTable.entries[transpositionTable.index(board)].value;
        }
        return ttEval;
    }

    if (depth == 0) {
        return quiescenceSearch(alpha, beta);
    }

    Movelist moves;
    movegen::legalmoves(moves, board);
    orderMoves(moves);

    if (moves.empty()) {
        if (board.inCheck()) {
            int mateScore = immediateMateScore - plyFromRoot;
            return -mateScore;
        } else {
            return 0;
        }
    }

    Move bestMoveHere = Move::NO_MOVE;
    int evalBound = TranspositionTable::upperBound;

    for (const Move move : moves) {
        board.makeMove(move);
        int eval = -searchMoves(depth - 1, plyFromRoot + 1, -beta, -alpha);
        board.unmakeMove(move);

        numNodes++;

        if (eval >= beta) {
            numCutoffs++;
            transpositionTable.storeEvaluation(depth, plyFromRoot, beta, TranspositionTable::lowerBound, move, board);
            return beta;
        }

        if (eval > alpha) {
            evalBound = TranspositionTable::exact;
            bestMoveHere = move;

            alpha = eval;
            if (plyFromRoot == 0) {
                bestMoveThisIteration = move;
                bestEvalThisIteration = eval;
            }
        }
    }

    transpositionTable.storeEvaluation(depth, plyFromRoot, alpha, evalBound, bestMoveHere, board);

    return alpha;
}

int Search::quiescenceSearch(int alpha, int beta) {
    int eval = evaluation.evaluate(board);
    searchDiagnostics.numPositionsEvaluated++;

    if (eval >= beta) {
        return beta;
    }
    if (eval > alpha) {
        alpha = eval;
    }

    Movelist moves;
    movegen::legalmoves(moves, board);
    Movelist captures;

    for (Move move : moves) {
	    if (board.isCapture(move)) {
	    captures.add(move);
	}
    }
    orderMoves(captures);

    for (const Move move : captures) {
        board.makeMove(move);
        eval = -quiescenceSearch(-beta, -alpha);
        board.unmakeMove(move);
        numQNodes++;

        if (eval >= beta) {
            numCutoffs++;
            return beta;
        }
        if (eval > alpha) {
            alpha = eval;
        }
    }

    return alpha;
}

void Search::orderMoves(Movelist moves) {
    sort(moves.begin(), moves.end(), [this](const Move& a, const Move& b) {
        bool aIsCapture = board.isCapture(a);
        bool bIsCapture = board.isCapture(b);
        
        if (aIsCapture != bIsCapture) {
            return aIsCapture > bIsCapture;
        }
        
        return false;
    });
}

bool Search::isMateScore(int score) {
    static constexpr int maxMateDepth = 1000;
    return abs(score) > immediateMateScore - maxMateDepth;
}

int Search::numPlyToMateFromScore(int score) {
    return immediateMateScore - abs(score);
}

void Search::initDebugInfo() {
    searchStartTime = chrono::steady_clock::now();
    numNodes = 0;
    numQNodes = 0;
    numCutoffs = 0;
    numTranspositions = 0;
}

void Search::logDebugInfo() {
    announceMate();
    auto searchDuration = chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now() - searchStartTime).count();
        
    printf("Best move: %s Eval: %d Search time: %lld ms\n", 
        uci::moveToUci(bestMoveThisIteration).c_str(), bestEvalThisIteration, searchDuration);
    printf("Num nodes: %d num Qnodes: %d num cutoffs: %d num TThits %d\n",
        numNodes, numQNodes, numCutoffs, numTranspositions);
    printf("Current TT size %zu\n", 
        transpositionTable.entries.size());
}

void Search::announceMate() {
    if (isMateScore(bestEvalThisIteration)) {
        int numPlyToMate = numPlyToMateFromScore(bestEvalThisIteration);
        int numMovesToMate = static_cast<int>(ceil(numPlyToMate / 2.0f));
        
        string sideWithMate = (bestEvalThisIteration * (board.sideToMove() == Color::WHITE ? 1 : -1) < 0) 
            ? "Black" : "White";

        printf("%s can mate in %d move%s\n", 
            sideWithMate.c_str(), numMovesToMate, (numMovesToMate > 1 ? "s" : ""));
    }
}
} // namespace chess
