#ifndef TRANSPOSITION_HPP
#define TRANSPOSITION_HPP

#include "chess.hpp"
#include <vector>
#include <mutex>
#include <unordered_map>

using namespace chess;

class TranspositionTable {
public:
    struct Entry {
        uint64_t key;       // Changed from size_t to match chess-library's hash type
        int value;
        uint8_t depth;
        uint8_t nodeType;
        Move move;

        Entry() : key(0), value(0), depth(0), nodeType(exact), move(Move::NO_MOVE) {}

        Entry(uint64_t key, int value, uint8_t depth, uint8_t nodeType, Move move)
            : key(key), value(value), depth(depth), nodeType(nodeType), move(move) {}
    };

    static const int lookupFailed = -1;
    static const int exact = 0;
    static const int lowerBound = 1;
    static const int upperBound = 2;

    TranspositionTable(uint64_t size);
    void clear();
    Move getStoredMove(uint64_t hash);
    int lookupEvaluation(int depth, int plyFromRoot, int alpha, int beta, uint64_t hash);
    void storeEvaluation(int depth, int numPlySearched, int eval, int evalType, Move move, uint64_t hash);

private:
    std::unordered_map<uint64_t, Entry> entries;
    std::mutex tableMutex;
    const uint64_t size_;
    bool enabled = true;

    uint64_t index(uint64_t hash) const;
    int correctMateScoreForStorage(int score, int numPlySearched);
    int correctRetrievedMateScore(int score, int numPlySearched);
};

#endif // TRANSPOSITION_HPP
