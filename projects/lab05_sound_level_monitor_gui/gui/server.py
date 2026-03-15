from flask import Flask, render_template, jsonify, send_file
import serial
import threading
import time
from collections import deque
from datetime import datetime
import csv
import os

app = Flask(__name__)

SERIAL_PORT = "COM3"   # change if needed
BAUD_RATE = 115200
MAX_POINTS = 200
LOG_FILE = "threshold_events.csv"

# Must match Arduino constants
CALIBRATION_OFFSET = 46.0   # tune in Arduino: calibrationOffset
REFERENCE_VOLTAGE  = 0.00631  # tune in Arduino: reference constant

sound_data = deque(maxlen=MAX_POINTS)
events = deque(maxlen=100)

current_value = 0.0
threshold = 70.0   # dB threshold — matches Arduino thresholdDB
alert_state = 0
last_alert_state = 0

ser = None
lock = threading.Lock()


def init_log_file():
    if not os.path.exists(LOG_FILE):
        with open(LOG_FILE, mode="", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["timestamp", "db_value", "threshold_db"])


def log_threshold_event(value: int, current_threshold: int):
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    with open(LOG_FILE, mode="a", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow([timestamp, value, current_threshold])


def connect_serial():
    global ser

    while True:
        try:
            print(f"Trying to connect to {SERIAL_PORT} at {BAUD_RATE} baud...")
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            time.sleep(2)
            print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud")
            return
        except Exception as e:
            print("Serial connection failed:", e)
            time.sleep(2)


def serial_reader():
    global current_value, threshold, alert_state, last_alert_state, ser

    connect_serial()

    while True:
        try:
            if ser is None or not ser.is_open:
                print("Serial port closed. Reconnecting...")
                connect_serial()

            line = ser.readline().decode("utf-8", errors="ignore").strip()

            if not line:
                continue

            print("RAW UART:", line)

            try:
                db_value = float(line)
            except ValueError:
                print("Ignored non-float line:", line)
                continue

            now_time = datetime.now().strftime("%H:%M:%S")
            full_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            with lock:
                current_value = db_value
                alert = 1 if db_value >= threshold else 0
                alert_state = alert

                sound_data.append({
                    "time": now_time,
                    "value": round(db_value, 1)
                })

                if alert_state == 1 and last_alert_state == 0:
                    event = {
                        "time": full_time,
                        "value": round(db_value, 1),
                        "threshold": threshold
                    }
                    events.appendleft(event)
                    log_threshold_event(round(db_value, 1), threshold)
                    print("THRESHOLD EVENT LOGGED:", event)

                last_alert_state = alert_state

        except serial.SerialException as e:
            print("Serial disconnected:", e)
            time.sleep(1)
            connect_serial()

        except Exception as e:
            print("Serial read error:", e)
            time.sleep(0.2)


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/download-csv")
def download_csv():
    abs_path = os.path.abspath(LOG_FILE)
    response = send_file(abs_path, mimetype="text/csv", as_attachment=True, download_name="threshold_events.csv")
    response.headers.add("Access-Control-Allow-Origin", "*")
    return response


@app.route("/data")
def get_data():
    with lock:
        payload = {
            "current_value": round(current_value, 1),
            "threshold": threshold,
            "alert": alert_state,
            "calibration_offset": CALIBRATION_OFFSET,
            "reference_voltage": REFERENCE_VOLTAGE,
            "data": list(sound_data),
            "events": list(events)
        }
    print("Serving /data:", payload["current_value"], payload["threshold"], payload["alert"], "points:", len(payload["data"]))
    response = jsonify(payload)
    response.headers.add("Access-Control-Allow-Origin", "*")
    return response


if __name__ == "__main__":
    init_log_file()

    reader_thread = threading.Thread(target=serial_reader, daemon=True)
    reader_thread.start()

    app.run(debug=True, use_reloader=False, host="0.0.0.0", port=5000)