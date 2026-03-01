# PortHive
A honeypot tool for the ESP32 that detects port scans and logs suspicious network activity to a dashboard.


Multiple ports are open on the Raspberry Pi, if all are hit within a short time interval, the activity is logged to the dashboard (IP, ports hit, time interval, SYN / full handshake)

Any further attempts at connecting to any of thse ports directly will start the honeypot, logging activity and data sent to the dashboard.


Things to add
- Make TCP server in C running on Pi
- Switch to raw sockets to capture raw packets (to detect SYN scans)
- Be memory and time efficient
- Aim for 3 open ports (TCP or HTTP) on the Pi
- Immediate connection reset (for full TCP handshake) and time between connnections between ports is too quick ? mark as suspicious, send log
- Even connection reset over long duration (eg. 5 min) from the same IP is suspicious
- Track SYN attacks
- Limit to 5 - 10 connections per port max (reduce memory usage)
- Throttle IP if the attacker is spamming any one of the ports to prevent running out of memory (wait x amount of time to accept new data and log / log and block the IP)
- Flask backend (make website localhost only, to prevent attacker from probing the dashboard)
- Flask frontend
