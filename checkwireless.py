from wireless_comm import get_distance, send_command, is_connected
import time

time.sleep(3)  # give it a moment to connect
print("Connected:", is_connected())

for _ in range(10):
    print("Distance:", get_distance())
    time.sleep(0.5)

send_command("LEFT")
time.sleep(1)
send_command("NONE")