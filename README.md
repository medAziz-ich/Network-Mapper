# Network Mapper (nmzyz)

## 📌 Description
`nmzyz` is a simple **network mapper** that scans a range of IP addresses and ports to identify open services.  
It works by attempting **TCP connections** and reporting which ports are open.  

## 🚀 Features
- Scan **a single IP or a range of IPs**  
- Scan **specific ports or a range of ports**  
- Uses **multi-threading** for faster scans  
- Detects **open and closed ports**  

---

## 🛠️ Installation
### **1️⃣ Compile the program**
Run:
  bash


##Usage
Basic Scan (Localhost)
  `./nmzyz 127.0.0.1 127.0.0.1 20 100`

Scan an Entire Subnet
  `./nmzyz 192.168.1.1 192.168.1.254 1 1024`
Scan a Public Server
  `./nmzyz scanme.nmap.org scanme.nmap.org 22 443`

Configuration

If you need to modify the code:

    Edit nmzyz.c for custom behavior
    Modify Makefile for compiler options

🧹 Cleanup

To remove compiled files:
  `make clean`



