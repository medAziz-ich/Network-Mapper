# Network Mapper (nmzyz)

`nmzyz` is a small multi-threaded **TCP connect scanner**. It scans a range of
IP addresses and ports and reports which ports accept connections.

> Only scan hosts you own or have explicit permission to test.

## Features

- Scan a single host or a range of IPv4 addresses
- Scan one port or a range of ports (1-65535)
- Accepts IP addresses **or hostnames**
- Thread pool for fast scans (configurable)
- Real per-connection timeout (non-blocking connect)
- Distinguishes **open**, **closed** (refused) and **filtered** (no answer)

## Build

Requires a C compiler with POSIX threads (Linux/macOS).

```bash
make
```

## Usage

```
./nmzyz [-t threads] [-w timeout_ms] [-v] <start IP|host> <end IP|host> <start port> <end port>
```

| Option | Meaning | Default |
| ------ | ------- | ------- |
| `-t N` | Number of worker threads (max 1024) | 100 |
| `-w MS` | Timeout per connection, in milliseconds | 1000 |
| `-v` | Also print closed and filtered ports | off |

Examples:

```bash
# Localhost, ports 20-100
./nmzyz 127.0.0.1 127.0.0.1 20 100

# A whole subnet (your own network!), ports 1-1024
./nmzyz 192.168.1.1 192.168.1.254 1 1024

# A host that permits scanning, verbose output
./nmzyz -v scanme.nmap.org scanme.nmap.org 22 443
```

Sample output:

```
[+] 127.0.0.1:8081 open
[+] 127.0.0.1:8082 open
```

Open ports go to stdout; the summary goes to stderr.

## Clean up

```bash
make clean
```
