from flask import Flask, request, jsonify
import socket
import requests

app = Flask(__name__)
FLASK_PORT = 5000
latency_times = []

@app.route("/", methods=["GET"])
def index() -> str:
	if len(latency_times) == 2:
		latency_ms = round((latency_times[1] - latency_times[0]) / 2)
		return f"<h1>Port Hive</h1><h2>Latency: {str(latency_ms)} ms"

	else:
		return f"<h1>Port Hive</h1><h2>"

@app.route("/latency", methods=["POST", "GET"])
def latency() -> str:
	if request.method == "POST":
		print("POST")
		system_time = request.get_json()
		if len(latency_times) == 2:
			latency_times.clear()
		
		latency_times.append(system_time["time"])

		return "LATENCY"

	else:
		return "<h1>Latency</h1>"


def get_ip() -> str:
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	s.connect(("8.8.8.8", 2000)) # Connect to google
	return s.getsockname()[0]

if __name__ == "__main__":
	ip = get_ip()
	app.run(host=ip, port=FLASK_PORT, debug=True)