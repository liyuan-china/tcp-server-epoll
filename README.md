# TCP Echo Server (Single-Threaded)

A minimal single-threaded TCP echo server written in C. Listens on a specified port, accepts one client connection, echoes back any received data, and supports a `quit` command to close the connection gracefully.

## Features
- Single-threaded, blocking I/O
- Echo any message back to the client
- `quit` command to terminate the connection
- Reusable address option (`SO_REUSEADDR`)

## Requirements
- Linux environment (or WSL)
- GCC
- `telnet` or `netcat` for testing

## Compilation
```bash
gcc server_echo.c -o server
```

## Usage
```bash
./server
```
Server listens on port 8888 by default. You can change the `PORT` macro in the source code.


## Connect with telnet
```bash
telnet localhost 8888
```
Type any message and press Enter. The server will echo it back.
Type `quit` to close the connection.

## Example session
```text
$ telnet localhost 8888
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
hello
hello
world
world
quit
Connection closed by foreign host.
```

## Connect with netcat
```bash
nc localhost 8888
```

## Code Structure
- server_echo.c - main server implementation

## Limitations
Handles only one client at a time.
Blocking I/O; cannot handle multiple connections concurrently.

## License
MIT License

## Author
Xu Liyuan - [GitHub Profile] (https://github.com/liyuan-china)



