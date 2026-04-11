import socket
import threading


def handle_client(conn, addr):
	print (f"NEW CONNECTION [{str(addr)}]")

	while True:
		try:
			msg = conn.recv(1024)

		except Exception as e:
			print (e)

		else:
			if not msg:
				print ("Empty message, possible port scanning")

			else:	
				print(f"[{str(addr)}]: {msg}")

if __name__ == "__main__":
	server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	addr = (("localhost", 80))
	server.bind(addr)

	server.listen(5)

	while True:
		try:
			conn, addr = server.accept()

		except Exception as e:
			print(e)

		else:
			thread = threading.Thread(target=handle_client, args=(conn,addr))
			thread.start()

