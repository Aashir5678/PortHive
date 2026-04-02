import socket
import time
import string
import random

def generate_fake_http_response():
	res = ""

	# send 50 - 100 kb of data / 100 ms
	size = random.randint(1000, 10000)
	for i in range(size):
		res += (random.choice(string.ascii_lowercase + string.ascii_uppercase))

	return res, size

ip = input("Enter target server ip: ")
port = int(input("Enter target port: "))
ADDR = ((ip, port))

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(ADDR)
total_size = 0

while True:
	try:
		res, size = generate_fake_http_response()
		total_size += size
		client.send(res.encode("utf-8"))
		print(f"Total size: {str(total_size)}")
		time.sleep(0.1)

	except (BrokenPipeError, ConnectionResetError) as e:
		print("Server disconnected")
		break

	except KeyboardInterrupt:
		print("Disconnecting...")
		break