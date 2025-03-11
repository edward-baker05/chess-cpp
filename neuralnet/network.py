import tensorflow as tf
import numpy as np
import chess
import csv
from sklearn.model_selection import train_test_split

def fen_to_features(fen):
    """Convert a FEN string to NNUE input features."""
    try:
        board = chess.Board(fen)
    except ValueError as e:
        raise ValueError(f"Invalid FEN: {fen}") from e

    features = np.zeros(768 + 1, dtype=np.float32)  # 768 pieces + 1 active color
    
    # Piece positions
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece is None:
            continue
        
        color_offset = 0 if piece.color == chess.WHITE else 6
        piece_index = piece.piece_type - 1  # chess.PAWN=1 -> 0, ..., chess.KING=6 -> 5
        index = square * 12 + color_offset + piece_index
        features[index] = 1.0
    
    # Active color (white=1.0, black=0.0)
    features[768] = 1.0 if board.turn == chess.WHITE else 0.0
    return features

def clipped_relu(x):
    """Clipped ReLU activation (0 ≤ output ≤ 1)."""
    return tf.minimum(tf.maximum(x, 0), 1.0)

def build_model():
    """Construct the NNUE model architecture."""
    return tf.keras.Sequential([
        tf.keras.layers.InputLayer(shape=(769,)),
        tf.keras.layers.Dense(256, activation=clipped_relu, name='hidden1'),
        tf.keras.layers.Dense(32, activation=clipped_relu, name='hidden2'),
        tf.keras.layers.Dense(1, name='output')
    ])

def load_data(filename):
    """Load training data from CSV file."""
    X, y = [], []

    with open(filename, 'r') as f:
        reader = list(csv.reader(f))


        items = sum(1 for _ in reader) 
        for i, row in enumerate(reader):
            if i % 100_000 == 0:
                print(f"{i} rows read of {items} total")
            if i == 2_000_000:
                break
            try:
                features = fen_to_features(row[0])
                X.append(features)
                y.append(float(row[1]))
            except ValueError as e:
                continue

    return np.array(X), np.array(y)

def train_model(model, X_train, y_train, X_val, y_val, epochs=10, batch_size=32):
    """Train the model with validation."""
    model.compile(optimizer='adam', loss='mse')

    try:
        load_weights(model, 'nnue_weights.weights.h5')
    except Exception as e:
        pass

    history = model.fit(
        X_train, y_train,
        validation_data=(X_val, y_val),
        epochs=epochs,
        batch_size=batch_size,
        verbose=1
    )
    return history

def save_weights(model, filename):
    """Save model weights to file."""
    model.save_weights(filename)

def load_weights(model, filename):
    """Load model weights from file."""
    model.load_weights(filename)

def main():
    # Configuration
    EPOCHS = 500
    BATCH_SIZE = 512
    DATA_PATH = 'training.csv'
    MODEL_WEIGHTS = 'nnue_weights.weights.h5'

    # Load and prepare data
    X, y = load_data(DATA_PATH)

    print("DATA LOADED")
    input()

    X_train, X_val, y_train, y_val = train_test_split(X, y, test_size=0.2, shuffle=True)

    # Build and train model
    model = build_model()
    for _ in range(0, EPOCHS, 10):
        train_model(model, X_train, y_train, X_val, y_val, epochs=10, batch_size=BATCH_SIZE)
    
    # Save weights
    save_weights(model, MODEL_WEIGHTS)
    print(f"Model weights saved to {MODEL_WEIGHTS}")

    # Example of loading weights
    # new_model = build_model()
    # load_weights(new_model, MODEL_WEIGHTS)

if __name__ == '__main__':
    main()
