#ifndef TRANSPOSITION_HPP
#define TRANSPOSITION_HPP

#include "chess.hpp"
#include <vector>

using namespace chess;

class TranspositionTable {
public:
    struct Entry {
        size_t key;
        int value;
        uint8_t depth;
        uint8_t nodeType;
        Move move;

        Entry() = default;

        Entry(size_t key, int value, uint8_t depth, uint8_t nodeType, Move move)
            : key(key), value(value), depth(depth), nodeType(nodeType), move(move) {}

        static int getSize() {
            return sizeof(Entry);
        }
    };

    static const int lookupFailed = -1;
    static const int exact = 0;
    static const int lowerBound = 1;
    static const int upperBound = 2;

    std::unordered_map<uint64_t, Entry> entries;
    const uint64_t size_;
    bool enabled = true;
    Board board_;

    TranspositionTable(Board& board, uint64_t size);
    void clear();
    uint64_t index(Board board) const;
    Move getStoredMove(Board board);
    int lookupEvaluation(int depth, int plyFromRoot, int alpha, int beta, Board board);
    void storeEvaluation(int depth, int numPlySearched, int eval, int evalType, Move move, Board board);
    int correctMateScoreForStorage(int score, int numPlySearched);
    int correctRetrievedMateScore(int score, int numPlySearched);
};

#endif // TRANSPOSITION_HPP
