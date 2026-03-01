# PortHive
A honeypot tool for the ESP32 that detects port scans and logs suspicious network activity to a dashboard.


Multiple ports are open on the ESP32, if all are hit within a short time interval, the activity is logged to the dashboard (IP, ports hit, time interval, SYN / full handshake)

Any further attempts at connecting to any of thse ports directly will start the honeypot, logging activity and data sent to the dashboard.
