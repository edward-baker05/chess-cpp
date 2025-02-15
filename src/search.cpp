#include "search.hpp"
#include "evaluation.hpp"
#include "transposition.hpp"
#include <cmath>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

using namespace std;
namespace chess {

Search::Search(Board& board, AISettings settings) 
    : board(board)
    , settings(settings)
    , transpositionTable(1 << 20)
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
    currentRootFEN = board.getFen();
    transpositionTable.clear();

    bestEvalThisIteration = bestEval = 0;
    bestMoveThisIteration = bestMove = Move::NO_MOVE;
    currentIterativeSearchDepth = 0;
    abortSearch = false;
    searchDiagnostics = SearchDiagnostics();

    if (settings.useIterativeDeepening) {
        cout << "Using iterative deepening" << endl;
        int targetDepth = settings.useFixedDepthSearch ? settings.depth : numeric_limits<int>::max();
        auto start = chrono::steady_clock::now();

        for (int searchDepth = 1; searchDepth <= targetDepth; searchDepth++) {
            auto end = chrono::steady_clock::now();
            int elapsed = chrono::duration<double, milli>(end - start).count();
            cout << "Depth " << searchDepth - 1 << " complete in " << elapsed << endl;

            if (searchDepth >= 4 && elapsed > 2000) break;

            cout << "Searching depth " << searchDepth << endl;
            parallelSearch(searchDepth);

            if (abortSearch) break;
            currentIterativeSearchDepth = searchDepth;
            bestMove = bestMoveThisIteration;
            bestEval = bestEvalThisIteration;
        }
    } else {
        parallelSearch(settings.depth);
        bestMove = bestMoveThisIteration;
        bestEval = bestEvalThisIteration;
    }
}

std::pair<Move, int> Search::getSearchResult() const {
    return {bestMove, bestEval};
}

void Search::endSearch() {
    abortSearch = true;
}

void Search::parallelSearch(int depth) {
    Board rootBoard;
    rootBoard.setFen(currentRootFEN);
    Movelist moves;
    movegen::legalmoves(moves, rootBoard);
    orderMoves(moves, rootBoard);

    if (moves.empty()) return;

    mutex mtx;
    atomic<int> sharedAlpha = negativeInfinity;
    atomic<bool> stopFlag = false;
    vector<thread> workers;
    string rootFEN = currentRootFEN;

    Board primaryBoard;
    primaryBoard.setFen(rootFEN);
    primaryBoard.makeMove(moves[0]);
    int eval = -searchMoves(depth - 1, 1, -positiveInfinity, -sharedAlpha, primaryBoard);
    primaryBoard.unmakeMove(moves[0]);

    {
        lock_guard<mutex> lock(mtx);
        bestMoveThisIteration = moves[0];
        bestEvalThisIteration = eval;
        sharedAlpha = eval;
    }

    for (size_t i = 1; i < moves.size(); i++) {
        workers.emplace_back([this, i, depth, rootFEN, &moves, &mtx, &sharedAlpha, &stopFlag]() {
            if (stopFlag.load()) return;

            Board threadBoard;
            threadBoard.setFen(rootFEN);

            const Move move = moves[i];
            threadBoard.makeMove(move);

            int eval = -searchMoves(depth - 1, 1, -positiveInfinity, -sharedAlpha, threadBoard);
            threadBoard.unmakeMove(move);

            lock_guard<mutex> lock(mtx);
            if (eval > sharedAlpha) {
                sharedAlpha = eval;
                bestMoveThisIteration = move;
                bestEvalThisIteration = eval;
            }
        });
    }

    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }
}

int Search::searchMoves(int depth, int plyFromRoot, int alpha, int beta, Board& currentBoard) {
    if (abortSearch) return 0;
    if (depth == 0) return quiescenceSearch(alpha, beta, currentBoard);

    uint64_t hash = currentBoard.hash();
    int ttValue = transpositionTable.lookupEvaluation(depth, plyFromRoot, alpha, beta, hash);
    if (ttValue != TranspositionTable::lookupFailed) return ttValue;

    Movelist moves;
    movegen::legalmoves(moves, currentBoard);
    orderMoves(moves, currentBoard);

    if (moves.empty()) {
        return currentBoard.inCheck() ? -immediateMateScore + plyFromRoot : 0;
    }

    int nodeType = TranspositionTable::upperBound;
    Move bestMove = Move::NO_MOVE;

    for (const Move move : moves) {
        Board newBoard; 
        newBoard.setFen(currentBoard.getFen());
        newBoard.makeMove(move);

        int eval = -searchMoves(depth - 1, plyFromRoot + 1, -beta, -alpha, newBoard);
        numNodes++;

        if (eval >= beta) {
            numCutoffs++;
            transpositionTable.storeEvaluation(depth, plyFromRoot, beta, TranspositionTable::lowerBound, move, hash);
            return beta;
        }

        if (eval > alpha) {
            alpha = eval;
            nodeType = TranspositionTable::exact;
            bestMove = move;
        }
    }

    transpositionTable.storeEvaluation(depth, plyFromRoot, alpha, nodeType, bestMove, hash);
    return alpha;
}

int Search::quiescenceSearch(int alpha, int beta, Board& currentBoard) {
    int eval = evaluation.evaluate(currentBoard);
    searchDiagnostics.numPositionsEvaluated++;

    if (eval >= beta) return beta;
    if (eval > alpha) alpha = eval;

    Movelist moves;
    movegen::legalmoves(moves, currentBoard);
    Movelist captures;
    for (Move move : moves) {
        if (currentBoard.isCapture(move)) captures.add(move);
    }
    orderMoves(captures, currentBoard);

    for (const Move move : captures) {
        Board newBoard;
        newBoard.setFen(currentBoard.getFen());
        newBoard.makeMove(move);

        int qeval = -quiescenceSearch(-beta, -alpha, newBoard);
        numQNodes++;

        if (qeval >= beta) return beta;
        if (qeval > alpha) alpha = qeval;
    }

    return alpha;
}

void Search::orderMoves(Movelist& moves, Board& currentBoard) {
    stable_sort(moves.begin(), moves.end(), [this, &currentBoard](const Move& a, const Move& b) {
        bool aIsCapture = currentBoard.isCapture(a);
        bool bIsCapture = currentBoard.isCapture(b);
        
        if (aIsCapture != bIsCapture) return aIsCapture > bIsCapture;
        if (aIsCapture && bIsCapture) {
            return capturedPieceValue(currentBoard, a) > capturedPieceValue(currentBoard, b);
        }
        return false;
    });
}

int Search::capturedPieceValue(Board& board, Move move) {
    PieceType captured = board.at(move.to()).type();

    switch (captured) {
        case (uint8_t) PieceType::underlying::PAWN:
            return Evaluation::pawnValue;
        case (uint8_t) PieceType::underlying::KNIGHT:
            return Evaluation::knightValue;
        case (uint8_t) PieceType::underlying::BISHOP:
            return Evaluation::bishopValue;
        case (uint8_t) PieceType::underlying::ROOK:
            return Evaluation::rookValue;
        case (uint8_t) PieceType::underlying::QUEEN:
            return Evaluation::queenValue;
    }
    return 0;
}

bool Search::isMateScore(int score) {
    static constexpr int maxMateDepth = 1000;
    return std::abs(score) > immediateMateScore - maxMateDepth;
}

int Search::numPlyToMateFromScore(int score) {
    return immediateMateScore - std::abs(score);
}

void Search::initDebugInfo() {
    searchStartTime = std::chrono::steady_clock::now();
    numNodes = 0;
    numQNodes = 0;
    numCutoffs = 0;
    numTranspositions = 0;
}

void Search::logDebugInfo() {
    announceMate();
    auto searchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - searchStartTime).count();
        
    printf("Best move: %s Eval: %d Search time: %lld ms\n", 
        uci::moveToUci(bestMoveThisIteration).c_str(), bestEvalThisIteration, searchDuration);
    printf("Num nodes: %d num Qnodes: %d num cutoffs: %d num TThits %d\n",
        numNodes, numQNodes, numCutoffs, numTranspositions);
}

void Search::announceMate() {
    if (isMateScore(bestEvalThisIteration)) {
        int numPlyToMate = numPlyToMateFromScore(bestEvalThisIteration);
        int numMovesToMate = static_cast<int>(std::ceil(numPlyToMate / 2.0f));
        
        std::string sideWithMate = (bestEvalThisIteration * (board.sideToMove() == Color::WHITE ? 1 : -1) < 0) 
            ? "Black" : "White";

        printf("%s can mate in %d move%s\n", 
            sideWithMate.c_str(), numMovesToMate, (numMovesToMate > 1 ? "s" : ""));
    }
}
} // namespace chess
