import tkinter as tk
import time
import tkinter.font as tkFont
import sys
import ctypes

if sys.platform == "win32":
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(1) 
    except (AttributeError, ImportError):
        pass

def update_time():
    current_time = time.localtime()
    hour = current_time.tm_hour
    minute = current_time.tm_min
    second = current_time.tm_sec

    time_string = time.strftime("%H:%M:%S")
    clock_label.config(text=time_string)

    if hour == 0:
        clock_label.config(fg="#086d28")
    elif hour == 1 and minute == 0 and second == 0:
        clock_label.config(fg="#249aff")
    else:
        clock_label.config(fg="#249aff")

    window.after(1000, update_time)

current_display_font = None

def on_resize(event):
    global current_display_font

    window_width = window.winfo_width()
    window_height = window.winfo_height()

    if window_width < 1 or window_height < 1:
        return

    font_size_from_height = int(window_height / 1.5)

    font_size_from_width = int(window_width / 4.5)

    new_font_size = min(font_size_from_height, font_size_from_width)

    if new_font_size < 1:
        new_font_size = 1

    new_font = tkFont.Font(family="Arial", size=new_font_size, weight="bold")
    clock_label.config(font=new_font)
    
    current_display_font = new_font


window = tk.Tk()
window.title("P3 風格時鐘")
window.geometry("800x400")

try:
    window.tk.call('tk', 'scaling', '-displayof', '.', '1')
    window.tk.call('tk', 'fontchooser', 'configure', '-antialias', True)
except tk.TclError:
    pass

window.configure(bg="black")

clock_label = tk.Label(
    window,
    font=("Arial", 100, "bold"),
    bg="black",
    fg="#249aff"
)
clock_label.pack(expand=True, fill="both")

window.bind("<Configure>", on_resize)

update_time()

on_resize(None) 

window.mainloop()