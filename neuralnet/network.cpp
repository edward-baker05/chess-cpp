#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <cassert>
#include "chess.hpp" // Include the chess-library header

using namespace chess;

// Constants for NNUE architecture
namespace NNUE {
    constexpr int SQUARE_NB = 64;
    constexpr int PIECE_TYPE_NB = 6; // Pawn, Knight, Bishop, Rook, Queen, King
    constexpr int COLOR_NB = 2;      // White, Black
    constexpr int PIECE_NB = PIECE_TYPE_NB * COLOR_NB;
    
    // Input layer size for HalfKP (King + Piece) feature transformer
    constexpr int FEATURE_DIMENSIONS = SQUARE_NB * SQUARE_NB * (PIECE_TYPE_NB * 2 - 1); // All pieces except kings

    // Network architecture parameters
    constexpr int HIDDEN_SIZE = 256;   // Size of hidden layer
    constexpr int OUTPUT_SIZE = 1;     // Single output for evaluation

    // Quantization parameters
    constexpr int QA = 255;
    constexpr int QB = 64;
    constexpr int SCALE = 400;
    
    // Activation function parameters
    constexpr int RELU_CLIP = QA;
}

// Feature Transformer for NNUE
class FeatureTransformer {
public:
    // Convert board position to feature indices
    std::vector<int> position_to_features(const Board& pos) {
        std::vector<int> features;
        features.reserve(32); // Maximum number of pieces on board
        
        Square white_king_square = pos.kingSq(Color::WHITE);
        Square black_king_square = pos.kingSq(Color::BLACK);
        
        // From white's perspective
        for (Square sq = Square(0); sq <= Square(63); ++sq) {
            Piece piece = pos.at(sq);
            if (piece != Piece::NONE && piece.type() != PieceType::KING) {
                int idx = calculate_index(Color::WHITE, white_king_square, sq, piece);
                if (idx >= 0) features.push_back(idx);
            }
        }
        
        // From black's perspective
        for (Square sq = Square(0); sq <= Square(63); ++sq) {
            Piece piece = pos.at(sq);
            if (piece != Piece::NONE && piece.type() != PieceType::KING) {
                int idx = calculate_index(Color::BLACK, black_king_square, sq, piece);
                if (idx >= 0) features.push_back(idx);
            }
        }
        
        return features;
    }

private:
    // Calculate feature index for HalfKP network
    int calculate_index(Color perspective, Square king_square, Square square, Piece piece) {
        if (piece == Piece::NONE || piece.type() == PieceType::KING)
            return -1;  // No feature for empty squares or kings
        
        int piece_type = static_cast<int>(piece.type());
        int color = static_cast<int>(piece.color());
        
        // Mirror king square for black's perspective
        if (perspective == Color::BLACK) {
            king_square = king_square.flip(); // Flip vertically
            square = square.flip();
            color = 1 - color;           // Swap color
        }
        
        return (king_square.index() * NNUE::SQUARE_NB * (NNUE::PIECE_TYPE_NB * 2 - 1)) +
               (square.index() * (NNUE::PIECE_TYPE_NB * 2 - 1)) +
               (piece_type + color * NNUE::PIECE_TYPE_NB);
    }
};

// Accumulator structure for NNUE
struct Accumulator {
    std::vector<int16_t> values;
    
    Accumulator(int size) : values(size, 0) {}
    
    void reset() {
        std::fill(values.begin(), values.end(), 0);
    }
};

class NNUENetwork {
private:
    // Network architecture parameters
    int input_size;
    int hidden_size;
    int output_size;

    // Network weights and biases
    std::vector<std::vector<int16_t>> weights_input;  // Input to hidden layer weights
    std::vector<int16_t> biases_input;               // Hidden layer biases
    std::vector<int16_t> weights_output;             // Hidden to output layer weights
    int16_t bias_output;                             // Output layer bias

    // Activation function (Clipped ReLU)
    int16_t activation(int16_t x) const {
        return std::max<int16_t>(0, std::min<int16_t>(x, NNUE::RELU_CLIP));
    }

public:
    // Constructor to initialize network dimensions and allocate weight matrices
    NNUENetwork(int input_dim = NNUE::FEATURE_DIMENSIONS,
                int hidden_dim = NNUE::HIDDEN_SIZE,
                int output_dim = NNUE::OUTPUT_SIZE)
        : input_size(input_dim),
          hidden_size(hidden_dim),
          output_size(output_dim),
          weights_input(input_dim, std::vector<int16_t>(hidden_dim)),
          biases_input(hidden_dim),
          weights_output(2 * hidden_dim),
          bias_output(0) {}

    // Initialize weights with random values
    void init_weights(unsigned seed = 42) {
        std::mt19937 rng(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f / std::sqrt(hidden_size));

        for (auto& row : weights_input) {
            for (auto& w : row) {
                w = static_cast<int16_t>(dist(rng) * NNUE::QA);
            }
        }

        for (auto& b : biases_input) {
            b = static_cast<int16_t>(dist(rng) * NNUE::QA);
        }

        for (auto& w : weights_output) {
            w = static_cast<int16_t>(dist(rng) * NNUE::QB);
        }

        bias_output = static_cast<int16_t>(dist(rng) * NNUE::QA * NNUE::QB);
    }

    // Compute hidden layer (accumulator) for given features
    void compute_accumulator(Accumulator& acc, const std::vector<int>& active_features) const {
        // Initialize with biases
        std::copy(biases_input.begin(), biases_input.end(), acc.values.begin());

        // Add weights for active features
        for (int feature_idx : active_features) {
            if (feature_idx >= 0 && feature_idx < input_size) {
                for (int j = 0; j < hidden_size; ++j) {
                    acc.values[j] += weights_input[feature_idx][j];
                }
            }
        }
    }

    // Update accumulator incrementally when features change
    void update_accumulator(Accumulator& acc,
                            const std::vector<int>& removed_features,
                            const std::vector<int>& added_features) const {
        // Subtract weights for removed features
        for (int feature_idx : removed_features) {
            if (feature_idx >= 0 && feature_idx < input_size) {
                for (int j = 0; j < hidden_size; ++j) {
                    acc.values[j] -= weights_input[feature_idx][j];
                }
            }
        }

        // Add weights for added features
        for (int feature_idx : added_features) {
            if (feature_idx >= 0 && feature_idx < input_size) {
                for (int j = 0; j < hidden_size; ++j) {
                    acc.values[j] += weights_input[feature_idx][j];
                }
            }
        }
    }

    // Forward evaluation using two accumulators (side to move and opponent)
    int forward(const Accumulator& stm_acc, const Accumulator& opp_acc) const {
        int32_t sum = 0;

        // Apply activation and dot product with output weights
        for (int i = 0; i < hidden_size; ++i) {
            sum += activation(stm_acc.values[i]) * weights_output[i];
            sum += activation(opp_acc.values[i]) * weights_output[hidden_size + i];
        }

        // Add output bias and scale
        sum += bias_output;
        sum = (sum * NNUE::SCALE) / (NNUE::QA * NNUE::QB);

        return sum;
    }

    // Save network weights to a binary file
    bool save_weights(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        // Write header with dimensions
        file.write(reinterpret_cast<const char*>(&input_size), sizeof(input_size));
        file.write(reinterpret_cast<const char*>(&hidden_size), sizeof(hidden_size));
        file.write(reinterpret_cast<const char*>(&output_size), sizeof(output_size));

        // Write input weights
        for (const auto& row : weights_input) {
            file.write(reinterpret_cast<const char*>(row.data()), row.size() * sizeof(int16_t));
        }

        // Write input biases
        file.write(reinterpret_cast<const char*>(biases_input.data()), biases_input.size() * sizeof(int16_t));

        // Write output weights
        file.write(reinterpret_cast<const char*>(weights_output.data()), weights_output.size() * sizeof(int16_t));

        // Write output bias
        file.write(reinterpret_cast<const char*>(&bias_output), sizeof(bias_output));

        return true;
    }

    // Load network weights from a binary file
    bool load_weights(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        // Read header with dimensions
        int in_size, hid_size, out_size;
        file.read(reinterpret_cast<char*>(&in_size), sizeof(in_size));
        file.read(reinterpret_cast<char*>(&hid_size), sizeof(hid_size));
        file.read(reinterpret_cast<char*>(&out_size), sizeof(out_size));

        // Validate dimensions
        if (in_size != input_size || hid_size != hidden_size || out_size != output_size) {
            std::cerr << "Network dimensions mismatch in file: "
                      << in_size << "x" << hid_size << "x" << out_size << std::endl;
            return false;
        }

        // Read input weights
        for (auto& row : weights_input) {
            file.read(reinterpret_cast<char*>(row.data()), row.size() * sizeof(int16_t));
        }

        // Read input biases
        file.read(reinterpret_cast<char*>(biases_input.data()), biases_input.size() * sizeof(int16_t));

        // Read output weights
        file.read(reinterpret_cast<char*>(weights_output.data()), weights_output.size() * sizeof(int16_t));

        // Read output bias
        file.read(reinterpret_cast<char*>(&bias_output), sizeof(bias_output));

        return true;
    }

    void train(const std::vector<std::vector<int>>& training_positions,
               const std::vector<float>& target_scores,
               int epochs = 10,
               float learning_rate = 0.01f,
               int batch_size = 1000) {

        std::vector<Accumulator> accumulators(2, Accumulator(hidden_size));
        size_t num_samples = training_positions.size();
        std::cout << "Training with " << num_samples << " datapoints" << std::endl;

        for (int epoch = 0; epoch < epochs; ++epoch) {
            float total_loss = 0.0f;

            for (size_t i = 0; i < num_samples; ++i) {
                const auto& features = training_positions[i];
                float target = target_scores[i];

                // Forward pass
                accumulators[0].reset();
                accumulators[1].reset();
                compute_accumulator(accumulators[0], features);
                std::cout << "Accumulators computed" << std::endl;

                int prediction = forward(accumulators[0], accumulators[1]);
                float sigmoid_pred = 1.0f / (1.0f + std::exp(-static_cast<float>(prediction) / 400.0f));

                // Compute loss (Mean Squared Error)
                float error = sigmoid_pred - target;
                total_loss += error * error;

                // Backward pass and weight update
                // Compute gradients and update weights
                // Note: This is a simplified version; actual implementation requires detailed gradient computation
                for (int j = 0; j < hidden_size; ++j) {
                    int16_t grad_output = static_cast<int16_t>(error * weights_output[j] / (NNUE::QA * NNUE::QB));
                    weights_output[j] -= static_cast<int16_t>(learning_rate * grad_output);

                    for (int k = 0; k < input_size; ++k) {
                        if (std::find(features.begin(), features.end(), k) != features.end()) {
                            int16_t grad_input = static_cast<int16_t>(error * weights_input[k][j] / NNUE::QA);
                            weights_input[k][j] -= static_cast<int16_t>(learning_rate * grad_input);
                        }
                    }
                }

                bias_output -= static_cast<int16_t>(learning_rate * error * NNUE::QB);

                std::cout << "Epoch " << epoch + 1 << ", Batch "
                          << (i / batch_size) + 1 << ", Loss: "
                          << total_loss / std::min(batch_size, static_cast<int>(i % batch_size + 1))
                          << std::endl;
                total_loss = 0.0f;
            }
        }
    }
};

// NNUE Evaluator class that combines the network and feature transformer
class NNUEEvaluator {
private:
    NNUENetwork network;
    FeatureTransformer feature_transformer;
    std::vector<Accumulator> accumulators;

public:
    NNUEEvaluator(int hidden_size = NNUE::HIDDEN_SIZE)
        : network(NNUE::FEATURE_DIMENSIONS, hidden_size, NNUE::OUTPUT_SIZE),
          accumulators(2, Accumulator(hidden_size)) {}

    // Initialize the network
    void init() {
        network.init_weights();
    }

    // Load network weights from file
    bool load_weights(const std::string& filename) {
        return network.load_weights(filename);
    }

    // Save network weights to file
    bool save_weights(const std::string& filename) {
        return network.save_weights(filename);
    }

    // Train the network
    void train(const std::vector<std::pair<std::string, float>>& training_data,
               int epochs = 10,
               float learning_rate = 0.01f,
               int batch_size = 1000) {

        std::vector<std::vector<int>> training_features;
        std::vector<float> target_scores;

        // Convert FEN positions to features
        for (const auto& [fen, score] : training_data) {
            Board pos;
            pos.setFen(fen);
            auto features = feature_transformer.position_to_features(pos);
            training_features.push_back(features);
            target_scores.push_back(score);
        }

        network.train(training_features, target_scores, epochs, learning_rate, batch_size);
    }

    // Evaluate a position from FEN
    int evaluate(const std::string& fen) {
        Board pos;

        // Extract active features
        auto features = feature_transformer.position_to_features(pos);

        // Reset accumulators
        accumulators[0].reset();
        accumulators[1].reset();

        // Compute accumulators
        network.compute_accumulator(accumulators[0], features);

        // Forward pass
        Color stm = pos.sideToMove();
        int stm_idx = (stm == Color::WHITE) ? 0 : 1;
        int nstm_idx = 1 - stm_idx;

        return network.forward(accumulators[stm_idx], accumulators[nstm_idx]);
    }
};

// Function to load training data from a CSV file
std::vector<std::pair<std::string, float>> load_training_data(const std::string& filename, size_t max_rows = 100000) {
    std::vector<std::pair<std::string, float>> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return data;
    }

    std::string line;
    size_t count = 0;
    while (std::getline(file, line) && count < max_rows) {
        std::stringstream ss(line);
        std::string fen;
        float evaluation;

        if (std::getline(ss, fen, ',') && ss >> evaluation) {
            data.emplace_back(fen, evaluation);
            ++count;
        }
    }

    return data;
}

int main() {
    try {
        // Initialize evaluator
        NNUEEvaluator evaluator;
        evaluator.init();

        // Load training data
        std::cout << "Loading training data..." << std::endl;
        auto training_data = load_training_data("training.csv");

        // Train the network
        std::cout << "Training network..." << std::endl;
        evaluator.train(training_data, 5);

        // Save the trained weights
        std::cout << "Saving weights..." << std::endl;
        evaluator.save_weights("nnue_weights.bin");

        // Load the weights and evaluate a position
        std::cout << "Loading weights..." << std::endl;
        if (evaluator.load_weights("nnue_weights.bin")) {
            std::string test_fen = "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3";  // After 1. e4 e5 2. Nf3 Nc6
            int eval = evaluator.evaluate(test_fen);

            std::cout << "Evaluation for position: " << test_fen << std::endl;
            std::cout << "Score: " << eval << " (in centipawns from white's perspective)" << std::endl;
        } else {
            std::cerr << "Failed to load weights" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
