#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AI 演示模式测试脚本
用于验证 C++ 和 Python 之间的通信是否正常
"""

import socket
import json
import time

def test_communication():
    """测试 UDP 通信"""

    # 创建 UDP Socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 8080))
    sock.settimeout(5.0)  # 5 秒超时

    print("=" * 60)
    print("AI 演示模式通信测试")
    print("=" * 60)
    print("\n[测试] 等待 C++ 客户端连接...")
    print("[提示] 请启动游戏并进入 AI 演示模式\n")

    try:
        # 等待接收第一条消息
        data, addr = sock.recvfrom(4096)
        print(f"✓ 成功接收到来自 {addr} 的数据")

        # 解析 JSON
        game_state = json.loads(data.decode('utf-8'))
        print(f"✓ JSON 解析成功")

        # 显示游戏状态
        print("\n接收到的游戏状态：")
        print(f"  自机位置: ({game_state['hero']['x']:.1f}, {game_state['hero']['y']:.1f})")
        print(f"  自机血量: {game_state['hero']['hp']}")
        print(f"  敌人数量: {len(game_state['enemies'])}")
        print(f"  子弹数量: {len(game_state['bullets'])}")

        # 发送测试指令
        test_command = {
            "dx": 0.5,
            "dy": -0.5,
            "fire": 1,
            "bomb": 0
        }

        response = json.dumps(test_command)
        cpp_addr = (addr[0], 9090)
        sock.sendto(response.encode('utf-8'), cpp_addr)
        print(f"\n✓ 已发送测试指令到 {cpp_addr}")
        print(f"  指令内容: {test_command}")

        print("\n" + "=" * 60)
        print("通信测试成功！")
        print("=" * 60)
        print("\n现在可以关闭此脚本，运行 ai_server.py 进行正式测试。")

    except socket.timeout:
        print("\n✗ 超时：未收到 C++ 客户端的数据")
        print("  请确保：")
        print("  1. 游戏已启动")
        print("  2. 已进入 AI 演示模式")
        print("  3. 防火墙未阻止本地通信")

    except json.JSONDecodeError as e:
        print(f"\n✗ JSON 解析失败: {e}")
        print(f"  接收到的原始数据: {data.decode('utf-8')}")

    except Exception as e:
        print(f"\n✗ 发生错误: {e}")

    finally:
        sock.close()

if __name__ == "__main__":
    test_communication()
