import sys
import chess
import chess.svg
import io
import tkinter as tk
from PIL import Image, ImageTk
import subprocess
import traceback

def log_error(message):
    with open("chess_error.log", "a") as f:
        f.write(f"{message}\n")
        f.write(traceback.format_exc() + "\n")

class ChessGUI:
    def __init__(self, root, fen):
        try:
            self.root = root
            self.root.title("Chess GUI")
            self.board = chess.Board(fen)
            self.root.focus_force()
            self.selected_square = None
            
            self.main_frame = tk.Frame(root)
            self.main_frame.pack(padx=10, pady=10)
            
            self.board_frame = tk.Frame(self.main_frame)
            self.board_frame.pack()
            
            self.canvas = tk.Label(self.board_frame)
            self.canvas.pack()
            
            self.control_frame = tk.Frame(self.main_frame)
            self.control_frame.pack(pady=5)
            
            self.status_label = tk.Label(self.control_frame, text="Your turn")
            self.status_label.pack()
            
            self.entry = tk.Entry(self.control_frame)
            self.entry.pack(pady=5)
            
            self.canvas.bind("<Button-1>", self.on_click)
            
            self.entry.bind("<Return>", lambda e: self.get_move())
            
            self.root.after(100, lambda: self.entry.focus_set())
            
            self.update_board()
            
        except Exception as e:
            log_error(f"Error in GUI initialization: {str(e)}")
            raise

    def update_board(self):
        try:
            with open("board.svg", "w") as f:
                f.write(chess.svg.board(self.board, size=600))
            
            result = subprocess.run(
                ["rsvg-convert", "-o", "board.png", "board.svg", "-w", "600", "-h", "600"],
                capture_output=True,
                text=True
            )
            
            if result.returncode != 0:
                log_error(f"rsvg-convert error: {result.stderr}")
                raise Exception("Failed to convert SVG")

            image = Image.open("board.png")
            image = image.resize((600, 600), Image.Resampling.LANCZOS)
            image = ImageTk.PhotoImage(image)
            self.canvas.config(image=image)
            self.canvas.image = image
        except Exception as e:
            log_error(f"Error in update_board: {str(e)}")
            raise

    def on_click(self, event):
        try:
            square_size = 600 // 8
            file = event.x // square_size
            rank = 7 - (event.y // square_size)
            clicked_square = chess.square(file, rank)
            
            if self.selected_square is None:
                # Check if the clicked square has a piece and it's that player's turn
                piece = self.board.piece_at(clicked_square)
                if piece and piece.color == self.board.turn:
                    self.selected_square = clicked_square
                    self.status_label.config(text="Select destination square")
                    self.update_board()
                else:
                    self.status_label.config(text="Select a valid piece")
            else:
                move = chess.Move(self.selected_square, clicked_square)
                # Check if it's a promotion move
                if (self.board.piece_at(self.selected_square).piece_type == chess.PAWN and 
                    ((self.board.turn and rank == 7) or (not self.board.turn and rank == 0))):
                    move = chess.Move(self.selected_square, clicked_square, chess.QUEEN)
                
                if move in self.board.legal_moves:
                    self.board.push(move)
                    self.update_board()
                    print(move.uci(), flush=True)
                    sys.exit(0)  # Exit after successful move
                else:
                    self.status_label.config(text="Invalid move! Try again")
                    self.update_board()  # Remove highlighting
                self.selected_square = None
        except Exception as e:
            log_error(f"Error in on_click: {str(e)}")
            raise

    def get_move(self, event=None):
        try:
            move_san = self.entry.get().strip()
            if not move_san:  # If empty, do nothing
                return
                
            try:
                move = self.board.parse_san(move_san)
                if move in self.board.legal_moves:
                    self.board.push(move)
                    self.update_board()
                    print(move.uci(), flush=True)
                    sys.exit(0)  # Exit after successful move
                else:
                    self.status_label.config(text="Invalid move! Try again")
                    self.entry.select_range(0, tk.END)  # Select all text for easy correction
            except ValueError:
                self.status_label.config(text="Invalid move format! Try again")
                self.entry.select_range(0, tk.END)  # Select all text for easy correction
        except Exception as e:
            log_error(f"Error in get_move: {str(e)}")
            raise

def main():
    try:
        if len(sys.argv) < 2:
            print("Error: FEN string required", file=sys.stderr)
            sys.exit(1)
        
        fen = sys.argv[1]
        root = tk.Tk()
        gui = ChessGUI(root, fen)
        root.mainloop()
    except Exception as e:
        log_error(f"Error in main: {str(e)}")
        print(f"Error: {str(e)}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
