import socket
import struct
import time

s = socket.socket()
s.connect(("127.0.0.1", 8888))

payload = b"hello_half"
frame = b'\xAB\xCD' + struct.pack('>H', len(payload)) + payload

# 先发送前半：帧头+长度+载荷前3字节
half_idx = 4 + 3   # 4字节帧头，3字节载荷
first_part = frame[:half_idx]
second_part = frame[half_idx:]

s.sendall(first_part)
print("发送前半帧:", first_part.hex())

time.sleep(0.5)    # 确保服务器已经处理过前半部分

s.sendall(second_part)
print("发送后半帧:", second_part.hex())

# 接收完整回显
data = s.recv(1024)
print("收到回显:", data.hex())

s.close()