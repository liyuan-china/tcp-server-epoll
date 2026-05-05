import socket
import struct

s = socket.socket()
s.connect(("127.0.0.1", 8888))

# 一次性发送3个粘在一起的完整帧
frames = []
for msg in [b"hello", b"world", b"miao"]:
    frame = b'\xAB\xCD' + struct.pack('>H', len(msg)) + msg
    frames.append(frame)
# 粘在一起发送
s.sendall(b''.join(frames))

# 接收3次回显（可能也是一次性到达，需要处理粘包接收）
received = []
while len(received) < 3:
    data = s.recv(1024)
    if not data:
        print("连接关闭，只收到", len(received), "个回显")
        break
    # 收到的数据可能也是多帧粘在一起的，但我们只验证收到了完整帧
    received.append(data)
    # 简单解析：查找帧头，按长度拆分（测试脚本可以依赖服务器返回的仍然是帧格式）
    # 这里简单打印原始字节
    print(f"收到回显原始数据: {data.hex()}")

s.close()
print("测试完成，共收到", len(received), "个回显块")