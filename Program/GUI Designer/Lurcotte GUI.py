import random
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from tkinter import Tk, Canvas, PhotoImage, Scrollbar, Frame, Button, Text
from pathlib import Path
from datetime import datetime
import time
import serial
import threading

# Serial connection setup
ser = None
serial_status = "❌ Serial not connected"
try:
    ser = serial.Serial('COM11', baudrate=9600, timeout=1)
    serial_status = "✅ Serial connected!"
except Exception as e:
    serial_status = f"❌ Failed to connect serial: {e}"

# Path setup
OUTPUT_PATH = Path(__file__).parent
ASSETS_PATH = OUTPUT_PATH / Path(r"E:\semester 4\mikrokontroller\soni2\build\assets\frame0")

def relative_to_assets(path: str) -> Path:
    return ASSETS_PATH / Path(path)

# Global variables
power_watt = 900  # Asumsikan daya lampu
total_energy_kwh = 0.00  # Akan diupdate dari mikrokontroller
biaya_rupiah = 0         # Akan diupdate dari mikrokontroller
last_time = time.time()
kwh_text_id = None
cost_text_id = None   # ID teks biaya listrik
current_voltage = 0.0  # Variabel untuk menyimpan nilai tegangan terakhir
serial_error_count = 0  # Menghitung error baca serial
current_value = 0.0  # Variabel untuk menyimpan nilai arus terakhir

# Main window setup
window = Tk()
window.title("Smart Home : Electronic Safety & Control System")
window.configure(bg="#F7F9FB")
window.geometry("1440x800")
window.resizable(True, True)

# Canvas and Scrollbar setup
main_canvas = Canvas(window, bg="#F7F9FB", highlightthickness=0)
main_canvas.pack(side="left", fill="both", expand=True)

v_scroll = Scrollbar(window, orient="vertical", command=main_canvas.yview)
v_scroll.pack(side="right", fill="y")
main_canvas.configure(yscrollcommand=v_scroll.set)

frame = Frame(main_canvas, bg="#F7F9FB")
main_canvas.create_window((0, 0), window=frame, anchor="nw")

def on_frame_configure(event):
    main_canvas.configure(scrollregion=main_canvas.bbox("all"))
frame.bind("<Configure>", on_frame_configure)

# Canvas UI design
canvas = Canvas(
    frame,
    bg="#F7F9FB",
    height=1024,
    width=1440,
    bd=0,
    highlightthickness=0,
    relief="ridge"
)
canvas.pack()

image_image_1 = PhotoImage(file=relative_to_assets("image_1.png"))
canvas.create_image(522.0, 676.0, image=image_image_1)

canvas.create_text(123.0, 30.0, anchor="nw", text="Smart Home : Electronic Safety & Control System", fill="#013A65", font=("Poppins Regular", 32 * -1))

image_image_2 = PhotoImage(file=relative_to_assets("image_2.png"))
canvas.create_image(1168.0, 183.0, image=image_image_2)

image_image_3 = PhotoImage(file=relative_to_assets("image_3.png"))
canvas.create_image(1166.0, 676.0, image=image_image_3)

canvas.create_rectangle(990.0, 307.0, 1346.0, 465.0, fill="#FFFFFF", outline="")
canvas.create_text(130.0, 105.0, anchor="nw", text="Voltage", fill="#FFFFFF", font=("Poppins Regular", 20 * -1))

image_image_4 = PhotoImage(file=relative_to_assets("image_4.png"))
canvas.create_image(1167.0, 379.0, image=image_image_4)

image_image_5 = PhotoImage(file=relative_to_assets("image_5.png"))
canvas.create_image(259.0, 698.0, image=image_image_5)

image_image_6 = PhotoImage(file=relative_to_assets("image_6.png"))
canvas.create_image(523.0, 698.0, image=image_image_6)

image_image_7 = PhotoImage(file=relative_to_assets("image_7.png"))
canvas.create_image(786.0, 698.0, image=image_image_7)

canvas.create_text(138.0, 529.0, anchor="nw", text="Manual Light Control", fill="#013A65", font=("Poppins Regular", 20 * -1))

canvas.create_text(113.0, 842.0, anchor="nw", text="Information", fill="#013A65", font=("Poppins Regular", 24 * -1))
canvas.create_text(217.0, 710.0, anchor="nw", text="Lampu ", fill="#013A65", font=("Poppins Regular", 24 * -1))
canvas.create_text(463.0, 710.0, anchor="nw", text="Elektronik 1", fill="#013A65", font=("Poppins Regular", 24 * -1))
canvas.create_text(725.0, 710.0, anchor="nw", text="Elektronik 2", fill="#013A65", font=("Poppins Regular", 24 * -1))

image_image_8 = PhotoImage(file=relative_to_assets("image_8.png"))
canvas.create_image(685.0, 940.0, image=image_image_8)

image_image_9 = PhotoImage(file=relative_to_assets("image_9.png"))
canvas.create_image(292.0, 940.0, image=image_image_9)

image_image_10 = PhotoImage(file=relative_to_assets("image_10.png"))
canvas.create_image(1075.0, 940.0, image=image_image_10)

canvas.create_text(989.0, 39.0, anchor="nw", text="Activity Log", fill="#013A65", font=("Poppins Regular", 32 * -1))

image_image_11 = PhotoImage(file=relative_to_assets("image_11.png"))
canvas.create_image(1348.0, 932.0, image=image_image_11)

canvas.create_text(990.0, 477.0, anchor="nw", text="Monitor", fill="#013A65", font=("Poppins Regular", 24 * -1))

status_text_id = canvas.create_text(
    1092.0, 153.0,
    anchor="nw",
    text="SAFE",
    fill="#00B6B6",
    font=("Poppins Regular", 40 * -1)
)

canvas.create_text(206.0, 890.0, anchor="nw", text="Household Power", fill="#013A65", font=("Poppins Regular", 20 * -1))
canvas.create_text(235.0, 932.0, anchor="nw", text="900 VA", fill="#013A65", font=("Poppins Regular", 36 * -1))

canvas.create_text(583.0, 890.0, anchor="nw", text="Power Consumption", fill="#013A65", font=("Poppins Regular", 20 * -1))

kwh_text_id = canvas.create_text(
    604.0, 932.0,
    anchor="nw",
    text=f"{total_energy_kwh:.2f} kWh",
    fill="#013A65",
    font=("Poppins Regular", 36 * -1)
)

image_image_12 = PhotoImage(file=relative_to_assets("image_12.png"))
canvas.create_image(1168.0, 385.0, image=image_image_12)

canvas.create_text(1011.0, 890.0, anchor="nw", text="Cost (Rupiah)  ", fill="#013A65", font=("Poppins Regular", 20 * -1))
cost_text_id = canvas.create_text(1001.0, 932.0, anchor="nw", text="Rp. 0", fill="#013A65", font=("Poppins Regular", 36 * -1))

canvas.create_text(1090.0, 320.0, anchor="nw", text="Reset kWh", fill="#013A65", font=("Poppins Regular", 32 * -1))
auto_mode_state = {"on": True}

# === Button Activity Log Section ===
button_log_frame = Frame(frame, bg="#F7F9FB")
button_log_frame.place(x=1000, y=95, width=340, height=169)

button_log_scrollbar = Scrollbar(button_log_frame)
button_log_scrollbar.pack(side="right", fill="y")

action_log_text = Text(
    button_log_frame,
    wrap="word",
    yscrollcommand=button_log_scrollbar.set,
    font=("Poppins", 10),
    bg="white",
    fg="black",
    bd=1,
    relief="solid"
)
action_log_text.pack(expand=True, fill="both")
button_log_scrollbar.config(command=action_log_text.yview)

def add_action_log(message):
    timestamp = datetime.now().strftime("%H:%M:%S")
    log_entry = f"  [{timestamp}] {message}\n"
    action_log_text.insert("end", log_entry)
    action_log_text.see("end")

def reset_energy():
    global total_energy_kwh, biaya_rupiah
    try:
        if ser and ser.is_open:
            ser.write(b'R')  # Kirim perintah reset ke Arduino
            time.sleep(0.5)  # Sedikit delay untuk memberi waktu mikrokontroler memproses
    except Exception as e:
        print(f"Error mengirim perintah reset: {e}")

    add_action_log("Reset nilai energi dan biaya (GUI + Arduino)")
    update_energy()
 

reset_button = Button(frame, text="", command=reset_energy, bg="#00B6B6", fg="white", font=("Poppins", 12), relief="flat")
reset_button.place(x=1080, y=398, width=180, height=40)

# Manual control states
lampu_state = {"on": False}
elektronik1_state = {"on": False}
elektronik2_state = {"on": False}

def flash_button(button, final_bg):
    button.config(bg="#FFFF66")
    window.after(100, lambda: button.config(bg=final_bg))

def toggle_lampu():
    lampu_state["on"] = not lampu_state["on"]
    state_text = "ON" if lampu_state["on"] else "OFF"
    final_color = "#00CC66" if lampu_state["on"] else "#FF4C4C"
    lampu_button.config(text=f"Lampu: {state_text}")
    flash_button(lampu_button, final_color)
    
    # Kirim perintah ke mikrokontroller
    if ser and ser.is_open:
        try:
            if lampu_state["on"]:
                ser.write(b'N')  # Ganti 13 dengan pin yang sesuai
                add_action_log("Mikrokontroller: Set PIN13 HIGH")
            else:
                ser.write(b'F')   # Ganti 13 dengan pin yang sesuai
                add_action_log("Mikrokontroller: Set PIN13 LOW")
        except Exception as e:
            add_action_log(f"Error: Gagal mengirim perintah ({str(e)})")
    else:
        add_action_log("Error: Port serial tidak terhubung")
    
    add_action_log(f"Lampu {state_text}")

def toggle_elektronik1():
    elektronik1_state["on"] = not elektronik1_state["on"]
    state_text = "ON" if elektronik1_state["on"] else "OFF"
    final_color = "#00CC66" if elektronik1_state["on"] else "#FF4C4C"
    elektronik1_button.config(text=f"Elektronik 1: {state_text}")
    flash_button(elektronik1_button, final_color)
    add_action_log(f"Manual Control: Elektronik 1 {state_text}")

def toggle_elektronik2():
    elektronik2_state["on"] = not elektronik2_state["on"]
    state_text = "ON" if elektronik2_state["on"] else "OFF"
    final_color = "#00CC66" if elektronik2_state["on"] else "#FF4C4C"
    elektronik2_button.config(text=f"Elektronik 2: {state_text}")
    flash_button(elektronik2_button, final_color)
    add_action_log(f"Manual Control: Elektronik 2 {state_text}")

lampu_button = Button(frame, text="Lampu: OFF", bg="#FF4C4C", fg="white", font=("Poppins", 12), relief="flat", command=toggle_lampu)
lampu_button.place(x=190, y=760, width=140, height=40)

elektronik1_button = Button(frame, text="Elektronik 1: OFF", bg="#FF4C4C", fg="white", font=("Poppins", 12), relief="flat", command=toggle_elektronik1)
elektronik1_button.place(x=440, y=760, width=180, height=40)

elektronik2_button = Button(frame, text="Elektronik 2: OFF", bg="#FF4C4C", fg="white", font=("Poppins", 12), relief="flat", command=toggle_elektronik2)
elektronik2_button.place(x=700, y=760, width=180, height=40)

# Voltage monitoring
voltages = []
time_stamps = []

fig, ax = plt.subplots(figsize=(5, 3))
canvas_widget = FigureCanvasTkAgg(fig, master=frame)
canvas_widget.get_tk_widget().place(x=112, y=100, width=820, height=400)

# === Voltage Log Activity Section ===
log_frame = Frame(frame, bg="#F7F9FB")
log_frame.place(x=1005, y=527, width=330, height=290)

log_scrollbar = Scrollbar(log_frame)
log_scrollbar.pack(side="right", fill="y")

log_text = Text(
    log_frame,
    wrap="word",
    yscrollcommand=log_scrollbar.set,
    font=("Poppins", 10),
    bg="white",
    fg="black",
    bd=1,
    relief="solid"
)
log_text.pack(expand=True, fill="both")
log_scrollbar.config(command=log_text.yview)

def add_voltage_log(voltage, status):
    timestamp = datetime.now().strftime("%H:%M:%S")
    log_entry = f"  [{timestamp}] Tegangan: {voltage:.1f} V - Status: {status}\n"
    log_text.insert("end", log_entry)
    log_text.see("end")

def voltage_color(voltage):
    if voltage > 250:
        return 'red'
    elif voltage < 150:
        return 'red'
    elif 150 <= voltage <= 230:
        return 'yellow'
    elif 230 < voltage <= 250:
        return 'green'
    else:
        return 'red'

def read_serial_data():
    global current_voltage, current_value, total_energy_kwh, biaya_rupiah, serial_error_count
    
    while True:
        try:
            if ser and ser.in_waiting:
                line = ser.readline().decode('utf-8').strip()
                
                if line.startswith("V:") and ";" in line:
                    # Contoh: V:228.55;I:0.423;E:1.2745;C:1723.56
                    try:
                        parts = line.split(";")
                        data = {}
                        for part in parts:
                            key, val = part.split(":")
                            data[key] = float(val)

                        current_voltage = data.get("V", 0.0)
                        current_value = data.get("I", 0.0)
                        total_energy_kwh = data.get("E", 0.0)
                        biaya_rupiah = data.get("C", 0.0)
                        serial_error_count = 0
                    except Exception as parse_err:
                        print(f"Parsing error: {parse_err}")

        except Exception as e:
            print(f"Serial error: {e}")
            serial_error_count += 1
            if serial_error_count > 5:
                current_voltage = 0.0
                current_value = 0.0
        
        time.sleep(0.1)


def update_voltage_graph():
    global voltages, time_stamps, current_voltage
    
    current_time = datetime.now().strftime("%H:%M:%S")
    
    # Gunakan nilai dari mikrokontroller atau 0 jika tidak ada koneksi
    if ser and ser.is_open and current_voltage > 0:
        new_voltage = current_voltage
    else:
        new_voltage = 0.0
    
    voltages.append(new_voltage)
    time_stamps.append(current_time)

    if len(voltages) > 6:
        voltages = voltages[1:]
        time_stamps = time_stamps[1:]

    ax.clear()
    ax.plot(time_stamps, voltages, label='Tegangan PLN', color='blue')

    for i in range(len(voltages)):
        dot_color = voltage_color(voltages[i])
        ax.scatter(time_stamps[i], voltages[i], color=dot_color, zorder=5)

    ax.set_xlabel('Waktu')
    ax.set_ylabel('Tegangan (V)')
    ax.set_title('Monitoring Tegangan PLN')
    ax.set_ylim(0, 300)
    ax.legend(loc='upper left')

    last_voltage = voltages[-1]
    dot_color = voltage_color(last_voltage)

    # Tentukan status dan warna
    if not ser or not ser.is_open or current_voltage <= 0:
        status = " NO DATA"
    elif dot_color == "green":
        status = "SAFE"
    elif dot_color == "yellow":
        status = "ALERT"
    else:
        status = "DANGER"

    add_voltage_log(last_voltage, status)
    canvas_widget.draw()
    window.after(1000, update_voltage_graph)

def update_energy():
    # Sekarang hanya update tampilan, tidak menghitung nilai
    canvas.itemconfig(kwh_text_id, text=f"{total_energy_kwh:.2f} kWh")
    canvas.itemconfig(cost_text_id, text=f"Rp. {int(biaya_rupiah):,}".replace(",", "."))
    window.after(1000, update_energy)

# Start serial reading thread
serial_thread = threading.Thread(target=read_serial_data, daemon=True)
serial_thread.start()

add_action_log(serial_status)
update_energy()
update_voltage_graph()
window.mainloop()
