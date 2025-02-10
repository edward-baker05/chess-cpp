#include <iostream>
#include <cstdio>
#include <string>
#include "chess.hpp"
#include "engine.hpp"

std::string getMoveFromPython(const std::string& fen) {
    // Use python3 explicitly and handle spaces in path
    #ifdef _WIN32
        std::string command = "python \"gui.py\" \"" + fen + "\"";
    #else
        std::string command = "python3 \"gui.py\" \"" + fen + "\"";
    #endif
    
    FILE* pipe = popen(command.c_str(), "r");
    
    if (!pipe) {
        std::cerr << "Failed to open Python process." << std::endl;
        return "";
    }

    std::array<char, 128> buffer;
    std::string result;

    // Read the move from Python
    if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result = buffer.data();
        // Trim whitespace and newlines
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
    }

    pclose(pipe);
    return result;
}

bool isLegal(Board board, Move move) {
	Movelist moves;
	movegen::legalmoves(moves, board);
	for (const auto &currentMove : moves) {
		if (uci::moveToUci(currentMove) == uci::moveToUci(move)) {
			return true;
		}
	}
	return false;
}

int main() {
    Board board;
    Engine engine(4, board);

	char side;
	std::cout << "Enter the colour you wish to play (w)hite/(b)lack: ";
	std::cin >> side;

	if (side == 'w') {
		std::cout << "White selected" << std::endl;
	} else if (side == 'b') {
		std::cout << "Black selected" << std::endl;
        Move aiMove = engine.getMove(board);
        board.makeMove(aiMove);
	} else {
		std::cout << "Invalid colour" << std::endl;
		main();
		return 0;
	}

    while (true) {
        std::string fen = board.getFen();
        Move userMove = uci::uciToMove(board, getMoveFromPython(fen));

        if (isLegal(board, userMove)) {
            board.makeMove(userMove);
        } else {
            std::cerr << "Invalid move received!" << std::endl;
            continue;
        }

        // Check if the game is over
        auto gameResult = board.isGameOver();
        if (gameResult.first != GameResultReason::NONE) {
            break;
        }

        // Get AI move
        Move aiMove = engine.getMove(board);
        board.makeMove(aiMove);

        // Check if the game is over after AI move
        gameResult = board.isGameOver();
        if (gameResult.first != GameResultReason::NONE) {
            break;
        }
    }

	 auto gameOverResult = board.isGameOver();

	switch (gameOverResult.first) {
	case GameResultReason::NONE:
		std::cout << "The game is still ongoing." << std::endl;
		break;
	case GameResultReason::CHECKMATE:
		if (board.sideToMove() == chess::Color::WHITE) {
			std::cout << "Black wins by checkmate!" << std::endl;
		} else {
			std::cout << "White wins by checkmate!" << std::endl;
		}
		break;
	case GameResultReason::STALEMATE:
		std::cout << "The game is a stalemate (draw)." << std::endl;
		break;
	case GameResultReason::INSUFFICIENT_MATERIAL:
		std::cout << "The game is a draw by insufficient material." << std::endl;
		break;
	case GameResultReason::FIFTY_MOVE_RULE:
		std::cout << "The game is a draw by the fifty-move rule." << std::endl;
		break;
	case GameResultReason::THREEFOLD_REPETITION:
		std::cout << "The game is a draw by threefold repetition." << std::endl;
		break;
	default:
		std::cout << "The game has ended with an unknown result." << std::endl;
	}

    return 0;
}
