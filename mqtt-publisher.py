import serial
import json
import time
import paho.mqtt.client as mqtt
# import serial.tools.list_ports

# only for windows



# -------- MQTT CONFIG --------
broker = "54247221371943e1b4bf707c8f0defd2.s1.eu.hivemq.cloud"
port = 8883
username = "station"
password = "Station@1"
topic = "msc/iot/weather_station/data"

client = mqtt.Client()
client.username_pw_set(username, password)
client.tls_set()

client.connect(broker, port)
client.loop_start()  # important for stable connection

# -------- SERIAL CONFIG --------
SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 9600
RETRY_INTERVAL = 5  # seconds
device_id = 1

# -------- For windows --------
# def get_serial_port():
#     """Get the first available serial port on Windows"""
#     ports = serial.tools.list_ports.comports()
#     for port in ports:
#         if "COM5" in port.device:
#             print("Found serial port: {port.device}")
#             return port.device
#     raise Exception("No serial port found")


# SERIAL_PORT = get_serial_port()
print("Using serial port: {SERIAL_PORT}")

def connect_serial():
    """Attempt to connect to Arduino serial"""
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            print("Serial connected")
            return ser
        except serial.SerialException:
            print("Serial not available. Retrying in 5 seconds...")
            time.sleep(RETRY_INTERVAL)


# Initial connection
ser = connect_serial()

while True:
    try:
        line = ser.readline().decode('utf-8').strip()

        if not line:
            continue

        temp_str, hum_str, wind_str, pir_stutus_str, fan_status_str, fan_speed_str = line.split(',')

        data = {
            "temperature": float(temp_str),
            "humidity": float(hum_str),
            "wind_speed": float(wind_str),
            "pir_status": int(pir_stutus_str),
            "fan_status": int(fan_status_str),
            "fan_speed": int(fan_speed_str),
            "device_id": device_id
        }

        json_payload = json.dumps(data)

        client.publish(topic, json_payload, qos=1)
        print("Published:", json_payload)

    except serial.SerialException:
        print("Serial disconnected!")
        ser.close()
        ser = connect_serial()

    except (ValueError, IndexError):
        print("Invalid data received")

    except Exception as e:
        print("Unexpected error:", e)
        time.sleep(2)