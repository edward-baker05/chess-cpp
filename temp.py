import time
import pyautogui
from pynput import mouse


def on_click(x, y, button, pressed):
    global count, positions

    if button == mouse.Button.left and (x, y) not in positions:
        positions.append((x, y))
        print(f"Recorded position {count+1}: ({x}, {y})")
        count += 1

    if count == 64:
        return False

print("Move your cursor to desired positions and press Enter to record each position.")
total_clicks = 64

print(f"Click {total_clicks} times to record cursor positions.")
print("Press Ctrl+C to exit early.")

positions = []
count = 0

listener = mouse.Listener(on_click=on_click)
listener.start()
listener.join()
        
print("\nAll positions recorded!")
print("\nFinal coordinates array:")
print(positions)
