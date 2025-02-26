#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <atomic>
#include "engine.hpp"  // Your internal engine header file

std::atomic<bool> stop_search(false);
int num_threads = 1;
bool ponder = false;
bool limitStrength = false;
int elo = 2500;
bool showWDL = false;

void uci_loop() {
    std::string command;
    Board board;
    Engine engine(4, board);
    
    std::cout << "id name WardenBot" << std::endl;
    std::cout << "id author Edward Baker" << std::endl;
    std::cout << "option name Move Overhead type spin default 30 min 0 max 5000" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 16" << std::endl;
    std::cout << "option name Hash type spin default 64 min 1 max 1024" << std::endl;
    std::cout << "option name Ponder type check default false" << std::endl;
    std::cout << "option name UCI_LimitStrength type check default false" << std::endl;
    std::cout << "option name UCI_Elo type spin default 2500 min 1350 max 2850" << std::endl;
    std::cout << "option name UCI_ShowWDL type check default false" << std::endl;
    std::cout << "option name SyzygyPath type string default " << std::endl;
    std::cout << "uciok" << std::endl;
    
    while (std::getline(std::cin, command)) {
        std::istringstream iss(command);
        std::string token;
        iss >> token;
        
        if (token == "uci") {
            std::cout << "id name WardenBot" << std::endl;
            std::cout << "id author Edward Baker" << std::endl;
            std::cout << "option name Move Overhead type spin default 30 min 0 max 5000" << std::endl;
            std::cout << "option name Threads type spin default 1 min 1 max 16" << std::endl;
            std::cout << "option name Hash type spin default 64 min 1 max 1024" << std::endl;
            std::cout << "option name Ponder type check default false" << std::endl;
            std::cout << "option name UCI_LimitStrength type check default false" << std::endl;
            std::cout << "option name UCI_Elo type spin default 2500 min 1350 max 2850" << std::endl;
            std::cout << "option name UCI_ShowWDL type check default false" << std::endl;
            std::cout << "option name SyzygyPath type string default " << std::endl;
            std::cout << "uciok" << std::endl;
        } 
        else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } 
        else if (token == "setoption") {
            std::string nameToken;
            iss >> nameToken; // Skip "name"
            
            if (nameToken == "name") {
                // Read the option name (might contain spaces)
                std::string optionName;
                iss >> optionName;
                
                // Read additional name parts until "value" is found
                std::string part;
                while (iss >> part && part != "value") {
                    optionName += " " + part;
                }
                
                // Read the value
                std::string optionValue;
                std::getline(iss, optionValue);
                // Remove leading space if exists
                if (!optionValue.empty() && optionValue[0] == ' ') {
                    optionValue = optionValue.substr(1);
                }
                
                // Handle different options
                if (optionName == "Threads") {
                    try {
                        num_threads = std::stoi(optionValue);
                        // You would update your engine's thread count here
                    } catch (...) {
                        num_threads = 1;
                    }
                }
                else if (optionName == "Ponder") {
                    ponder = (optionValue == "true");
                }
                else if (optionName == "UCI_LimitStrength") {
                    limitStrength = (optionValue == "true");
                    // Update engine strength limiting
                }
                else if (optionName == "UCI_Elo") {
                    try {
                        elo = std::stoi(optionValue);
                        // Update engine's Elo cap
                    } catch (...) {
                        elo = 2500;
                    }
                }
                else if (optionName == "UCI_ShowWDL") {
                    showWDL = (optionValue == "true");
                }
            }
        } 
        else if (token == "ucinewgame") {
            board = Board(); // Reset to initial position
            Engine engine(4, board); // Reset engine with default settings
        } 
        else if (token == "position") {
            std::string pos_type;
            iss >> pos_type;
            
            if (pos_type == "startpos") {
                Board board;
                
                std::string moves_str;
                if (iss >> moves_str && moves_str == "moves") {
                    std::string move;
                    while (iss >> move) {
                        board.makeMove(uci::uciToMove(board, move));
                    }
                }
            }
            else if (pos_type == "fen") {
                std::string rest_of_line;
                std::getline(iss, rest_of_line); // Read the rest of the input line

                std::istringstream fen_stream(rest_of_line);
                std::string fen;
                std::string token;
                std::string moves_marker;

                // Read everything until "moves" is found
                while (fen_stream >> token) {
                    if (token == "moves") {
                        moves_marker = token;
                        break;
                    }
                    if (!fen.empty()) fen += " "; // Add space between FEN parts
                    fen += token;
                }

                std::cout << "Changing fen to " << fen << std::endl;
                Movelist moves;
                movegen::legalmoves(moves, board);
                bool updated = false;

                for (Move move : moves) {
                    board.makeMove(move);
                    Movelist moves2;
                    movegen::legalmoves(moves2, board);
                    for (Move move2 : moves2) {
                        board.makeMove(move2);
                        if (board.getFen() == fen) {
                            updated = true;
                            break;
                        }
                        board.unmakeMove(move2);
                    }
                    if (updated) break;
                    board.unmakeMove(move);
                }

                if (!updated) {
                    std::cout << "No valid route to position found" << std::endl;
                    board.setFen(fen);
                }

                // Handle moves after "moves"
                if (!moves_marker.empty()) {
                    std::string move;
                    while (fen_stream >> move) {
                        board.makeMove(uci::uciToMove(board, move));
                    }
                }
                engine.setPosition(board);
                std::cout << "Board position successfully set to " << board.getFen() << std::endl;
            }
        }
        else if (token == "go") {
            stop_search = false;
            
            // Launch search in a separate thread
            std::thread search_thread([&]() {
                Move bestMove = engine.getMove(board);
                if (!stop_search) {
                    std::cout << "bestmove " << uci::moveToUci(bestMove) << std::endl;
                }
            });
            search_thread.detach(); // Let it run independently
        } 
        else if (token == "stop") {
            stop_search = true;
            Move bestMove = engine.getMove(board);
            std::cout << "bestmove " << uci::moveToUci(bestMove) << std::endl;
        } 
        else if (token == "quit") {
            break;
        }
    }
}

int main() {
    uci_loop();
    return 0;
}
