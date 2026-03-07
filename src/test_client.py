import socket

ADDR = (("localhost", 5050))

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(ADDR)

while True:
	client.send(input("Send data to server: ").encode("utf-8"))