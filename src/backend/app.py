from flask import Flask
import socket

app = Flask(__name__)
FLASK_PORT = 5000

@app.route("/")
def index() -> str:
	return "Port Hive"


def get_ip() -> str:
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	s.connect(("8.8.8.8", 2000))
	return s.getsockname()[0]

if __name__ == "__main__":
	ip = get_ip()
	print (ip)
	app.run(host=ip, port=FLASK_PORT)