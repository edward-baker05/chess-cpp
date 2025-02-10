#include "transposition.hpp"
#include "search.hpp"

using namespace std;

TranspositionTable::TranspositionTable(Board& board, uint64_t size)
    : board_(board), size_(size), entries() {
    entries.rehash(size);
}

void TranspositionTable::clear() {
    entries.clear();
}

uint64_t TranspositionTable::index(Board board) const {
    return board.hash() % size_;
}

Move TranspositionTable::getStoredMove(Board board) {
    return entries[index(board)].move;
}

bool tryLookupEvaluation(int depth, int plyFromRoot, int alpha, int beta, int eval) {
    eval = 0;
    return false;
}

int TranspositionTable::lookupEvaluation(int depth, int plyFromRoot, int alpha, int beta, Board board) {
    if (!enabled) {
        return lookupFailed;
    }

    Entry entry = entries[index(board)];

    if (entry.key == board.hash()) {
        if (entry.depth >= depth) {
            int correctedScore = correctRetrievedMateScore(entry.value, plyFromRoot);
            if (entry.nodeType == exact) {
                return correctedScore;
            }
            if (entry.nodeType == upperBound && correctedScore <= alpha) {
                return correctedScore;
            }
            if (entry.nodeType == lowerBound && correctedScore >= beta) {
                return correctedScore;
            }
        }
    }
    return lookupFailed;
}

void TranspositionTable::storeEvaluation(int depth, int numPlySearched, int eval, int evalType, Move move, Board board) {
    if (!enabled) {
        return;
    }

    Entry entry(board.hash(), correctMateScoreForStorage(eval, numPlySearched), (uint8_t)depth, (uint8_t)evalType, move);
    entries[index(board)] = entry;
}

int TranspositionTable::correctMateScoreForStorage(int score, int numPlySearched) {
    if (Search::isMateScore(score)) {
        int sign = score > 0 ? 1 : score < 0 ? -1 : 0;
        return (score * sign + numPlySearched) * sign;
    }
    return score;
}

int TranspositionTable::correctRetrievedMateScore(int score, int numPlySearched) {
    if (Search::isMateScore(score)) {
        int sign = score > 0 ? 1 : score < 0 ? -1 : 0;
        return (score * sign - numPlySearched) * sign;
    }
    return score;
}
