#include "chess.hpp"
#include "evaluation.hpp"

using namespace chess;
using namespace std::chrono_literals;

int main() {
    Board board;

    Movelist moves;
    board::legalmoves(moves, board);

    for (Move move : moves) {


    }
}
