#include "transposition.hpp"
#include "search.hpp"

TranspositionTable::TranspositionTable(uint64_t size) : size_(size) {
    entries.rehash(size);
}

void TranspositionTable::clear() {
    std::lock_guard<std::mutex> lock(tableMutex);
    entries.clear();
}

uint64_t TranspositionTable::index(uint64_t hash) const {
    return hash % size_;
}

Move TranspositionTable::getStoredMove(uint64_t hash) {
    std::lock_guard<std::mutex> lock(tableMutex);
    auto it = entries.find(hash);
    return (it != entries.end()) ? it->second.move : Move::NO_MOVE;
}

int TranspositionTable::lookupEvaluation(int depth, int plyFromRoot, int alpha, int beta, uint64_t hash) {
    if (!enabled) return lookupFailed;

    std::lock_guard<std::mutex> lock(tableMutex);
    auto it = entries.find(hash);
    
    if (it != entries.end() && it->second.key == hash) {
        const Entry& entry = it->second;
        if (entry.depth >= depth) {
            int correctedScore = correctRetrievedMateScore(entry.value, plyFromRoot);
            
            if (entry.nodeType == exact) return correctedScore;
            if (entry.nodeType == upperBound && correctedScore <= alpha) return correctedScore;
            if (entry.nodeType == lowerBound && correctedScore >= beta) return correctedScore;
        }
    }
    return lookupFailed;
}

void TranspositionTable::storeEvaluation(int depth, int numPlySearched, int eval, 
                                       int evalType, Move move, uint64_t hash) {
    if (!enabled) return;

    std::lock_guard<std::mutex> lock(tableMutex);
    entries[hash] = Entry(
        hash,
        correctMateScoreForStorage(eval, numPlySearched),
        static_cast<uint8_t>(depth),
        static_cast<uint8_t>(evalType),
        move
    );
}

int TranspositionTable::correctMateScoreForStorage(int score, int numPlySearched) {
    if (Search::isMateScore(score)) {
        int sign = (score > 0) ? 1 : -1;
        return sign * (std::abs(score) + numPlySearched);
    }
    return score;
}

int TranspositionTable::correctRetrievedMateScore(int score, int numPlySearched) {
    if (Search::isMateScore(score)) {
        int sign = (score > 0) ? 1 : -1;
        return sign * (std::abs(score) - numPlySearched);
    }
    return score;
}
