import socket
import time
import string
import random

def generate_fake_http_response():
	res = ""

	# send 50 - 100 kb of data / 100 ms
	for i in range(random.randint(50000, 100000)):
		res += (random.choice(string.ascii_lowercase + string.ascii_uppercase))

	return res

ip = input("Enter target server ip: ")
port = int(input("Enter target port: "))
ADDR = ((ip, port))

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(ADDR)

while True:
	try:
		client.send(generate_fake_http_response().encode("utf-8"))
		time.sleep(0.1)

	except (BrokenPipeError, ConnectionResetError) as e:
		print("Server disconnected")
		break

	except KeyboardInterrupt:
		print("Disconnecting...")
		break