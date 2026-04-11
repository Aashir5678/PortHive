from flask import Flask, request, jsonify, redirect
from flask_cors import CORS
import socket
import time
import requests

app = Flask(__name__)
CORS(app)

FLASK_PORT = 5000
PORT_SCAN_TIME_THRESHOLD = 5 # time in seconds between connection and disconnect
latency_times = []

clients = []
port_scanning_clients = []
injection_attempts = []



class Client:
	def __init__(self, ipv4: str, port: int, time_connected_at: int):
		self.ipv4 = ipv4
		self.port = port
		self.connected = True
		self.time_connected_at = time_connected_at
		self.messages = []

	def __str__(self):
		return f"Client({self.ipv4}, {str(self.port)})"

	def get_time_connected(self) -> int:
		return (round(time.time() * 1000)) - self.time_connected_at

	def add_message(self, msg):
		self.messages.append(msg)

	def get_messages(self):
		return self.messages

	def update_time_connected(self, new_time: int) -> None:
		self.time_connected_ms = new_time


@app.route("/dashboard")
def dashboard():
	return app.send_static_file("index.html")

@app.route("/")
def index():
    return redirect("/dashboard")

@app.route("/latency", methods=["POST", "GET"])
def latency():
	if request.method == "POST":
		system_time = request.get_json()
		if len(latency_times) == 2:
			latency_times.clear()
		
		latency_times.append(system_time["time"])

		return "LATENCY", 200

	else:
		if len(latency_times) == 2:
			return jsonify({"latency": latency_times[1] - latency_times[0]})

		else:
			return jsonify({"latency": -1})

@app.route("/new_msg", methods=["POST"])
def new_client_msg():
	msg_data = request.get_json(force=True, silent=True)

	# Attempted injection
	if msg_data is None:
		print (f"Attempted injection")
		return "Injection attempt", 400

	for i in range(len(clients)):
		if clients[i].ipv4 == msg_data["ipv4"] and clients[i].port == msg_data["port"]:
			# SANATIZE JSON HERE
			clients[i].messages.append(msg_data["msg"])

			print (f"new msg from {clients[i].ipv4}: {msg_data["msg"]}")

			return "Success", 200


	return "Couldn't find client in request", 400
	
@app.route("/port_scanning_clients", methods=["GET"])
def get_port_scanning_clients():
	if len(port_scanning_clients) < 2:
		return {}

	first_instance = port_scanning_clients[0]
	last_instance = port_scanning_clients[-1]

	if (last_instance[1] - first_instance[1] < PORT_SCAN_TIME_THRESHOLD):
		clis = []
		for time_scanned_at, client in port_scanning_clients:
			clis.append({"ipv4": client.ipv4, "port": client.port, "time_connected_at": client.time_connected_at, "status": client.connected})

		return jsonify(clis)

	else:
		return {}

@app.route("/new", methods=["POST"])
def new_client():
	data = request.get_json()
	client = Client(data["ipv4"], int(data["port"]), int(data["time"]))
	clients.append(client)

	return "Success", 200


@app.route("/block", methods=["POST"])
def block_ip():
	pass

@app.route("/unblock", methods=["POST"])
def unblock_ip():
	pass

@app.route("/remove", methods=["POST"])
def remove_client():
	data = request.get_json()
	client = None


	for i in range(len(clients)):
		if clients[i].ipv4 == data["ipv4"] and clients[i].port == data["port"]:
			clients[i].connected = False


			if len(clients[i].get_messages()) == 0:
				port_scanning_clients.append((clients[i], time.time()))

			return "Success", 200

	else:
		return f"Couldn't find client with ip {data["ipv4"]}", 400


@app.route("/clients", methods=["GET"])
def get_clients():
	clis = []
	for client in clients:
		clis.append({"ipv4": client.ipv4, "port": client.port, "time_connected_at": client.time_connected_at, "status": client.connected, "messages": client.messages})

	return jsonify(clis)


def get_ip():
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	s.connect(("8.8.8.8", 2000)) # Connect to google
	return s.getsockname()[0]

if __name__ == "__main__":
	# ip = get_ip()
	app.run(host="localhost", port=FLASK_PORT, debug=True)
