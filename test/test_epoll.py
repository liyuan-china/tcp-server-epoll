import socket, threading, time

def test_nonblocking(host='127.0.0.1', port=8888, msg_size=10000):
    sock = socket.socket()
    sock.connect((host, port))
    msg = b'X' * msg_size
    sock.sendall(msg)
    sock.shutdown(socket.SHUT_WR)  # 关闭写方向，让服务器知道数据发完
    recv = sock.recv(msg_size)
    sock.close()
    assert recv == msg, f"Echo mismatch: sent {len(msg)} bytes, got {len(recv)}"
    print("Large message test passed")

if __name__ == '__main__':
    test_nonblocking()