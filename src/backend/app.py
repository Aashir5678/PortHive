from flask import Flask, request, jsonify
import socket
import time
import requests

app = Flask(__name__)
FLASK_PORT = 5000
latency_times = []

clients = []


class Client:
	def __init__(self, ipv4: str, port: int, time_connected_at: int):
		self.ipv4 = ipv4
		self.port = port
		self.time_connected_at = time_connected_at

	def get_time_connected(self) -> int:
		return (round(time.time() * 1000)) - self.time_connected_at


	def update_time_connected(self, new_time: int) -> None:
		self.time_connected_ms = new_time




@app.route("/", methods=["GET"])
def index():
	if len(latency_times) == 2:
		latency_ms = (latency_times[1] - latency_times[0])
		clients_list = ""
		for client in clients:
			clients_list += f"<h2>{client.ipv4} {str(client.port)}, connected for {str(client.get_time_connected())} ms</h2>"


		return f"<h1>Port Hive</h1><h2>Latency: {str(latency_ms)} ms</h2>" + clients_list

	else:
		return f"<h1>Port Hive</h1><h2>"

@app.route("/latency", methods=["POST"])
def latency():
	system_time = request.get_json()
	if len(latency_times) == 2:
		latency_times.clear()
	
	latency_times.append(system_time["time"])

	return "LATENCY", 200


@app.route("/new", methods=["POST"])
def new_client():
	data = request.get_json()
	client = Client(data["ipv4"], int(data["port"]), int(data["time"]))
	clients.append(client)

	return "Success", 200


@app.route("/remove", methods=["POST"])
def remove_client():
	data = request.get_json()
	client = None

	for cli in clients:
		if cli.ipv4 == data["ipv4"] and cli.port == int(data["port"]):
			client = cli
	
	if client is not None:
		clients.remove(client)
		return "Success", 200

	else:
		return f"Couldn't find client with ip {data["ipv4"]}", 400


def get_ip():
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	s.connect(("8.8.8.8", 2000)) # Connect to google
	return s.getsockname()[0]

if __name__ == "__main__":
	ip = get_ip()
	app.run(host=ip, port=FLASK_PORT, debug=True)