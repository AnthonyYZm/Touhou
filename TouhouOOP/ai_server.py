#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import json
import time
import threading
import urllib.request
import urllib.error
import ssl
import os
from dotenv import load_dotenv

load_dotenv()

# 配置区
LOCAL_IP = "127.0.0.1"
LOCAL_PORT = 8080
CPP_PORT = 9090

# 在这里添加API密钥
API_KEY = os.getenv("KIMI_API_KEY", "sk-placeholder") 
BASE_URL = os.getenv("KIMI_BASE_URL", "https://api.moonshot.cn/v1")
MODEL_NAME = os.getenv("KIMI_MODEL_NAME", "moonshot-v1-8k")

# 全局状态
global_game_state = {}
global_command = {
    "target_x": 375.0,  # 默认中心
    "target_y": 800.0,  # 默认底部
    "fire": 1
}
state_lock = threading.Lock()
command_lock = threading.Lock()

def llm_worker_thread():
    """
    后台思考线程：使用 Python 原生库直接发起 HTTP 请求
    """
    global global_game_state, global_command
    
    print("[LLM Thread] 大脑思考线程已启动...")
    
    while True:
        # 1. 安全提取当前状态
        with state_lock:
            if not global_game_state:
                time.sleep(0.5)
                continue
            state_copy = global_game_state.copy()
            
        hero = state_copy.get("hero", {})
        bullets = state_copy.get("bullets", [])
        boss = state_copy.get("enemies", [{}])[0] if state_copy.get("enemies") else {}

        # 2. 构建 Prompt
        system_prompt = """你是一个 2D 弹幕射击游戏的宏观战术AI。
你不能进行微操躲避（底层算法会自动避障），你的任务是根据战场局势，给出接下来的【目标安全坐标】。
游戏屏幕范围：X坐标(32 ~ 782)，Y坐标(30 ~ 930)。

战术指导：
1. 默认情况下，目标坐标应该保持在 Boss 的正下方 (X与Boss相同，Y保持在700~800)，以便于输出。
2. 如果自机周围有很多危险子弹，请给出远离弹幕密集区的安全区域坐标, 且优先考虑向屏幕中间移动防止被逼入屏幕边缘。
3. 如果没有特殊情况，应该尽量保持在屏幕下半部分活动，避免过早进入 Boss 的攻击范围。

你必须且只能返回合法的 JSON，格式如下：
{"target_x": 400.0, "target_y": 800.0, "fire": 1}
绝不要输出任何多余的解释文字！只能输出 JSON 对象！"""

        user_prompt = f"""
【当前战况】
自机位置: X={hero.get('x', 0):.1f}, Y={hero.get('y', 0):.1f}
Boss位置: X={boss.get('x', 0):.1f}, Y={boss.get('y', 0):.1f}
自机周围危险子弹数量: {len(bullets)}

请分析局势，如果子弹太多请指引远离密集区，如果安全请指引靠近 Boss 正下方输出,尽量保持在屏幕下半部分活动。输出你的 JSON 战术："""

        # 3. 构造原生 HTTP 请求调用 Kimi API
        try:
            print(f"[LLM Thread] 正在请求大模型思考战术... (子弹数: {len(bullets)})")
            start_time = time.time()
            
            # 组装请求数据
            req_data = {
                "model": MODEL_NAME,
                "messages": [
                    {"role": "system", "content": system_prompt},
                    {"role": "user", "content": user_prompt}
                ],
                "temperature": 0.2
            }
            req_body = json.dumps(req_data).encode('utf-8')
            
            # 设置请求头
            req = urllib.request.Request(
                f"{BASE_URL}/chat/completions",
                data=req_body,
                headers={
                    "Content-Type": "application/json",
                    "Authorization": f"Bearer {API_KEY}"
                }
            )
            
            # 发起请求并读取响应
            ssl_context = ssl._create_unverified_context()
            with urllib.request.urlopen(req, context=ssl_context) as response:
                res_body = response.read().decode('utf-8')
                res_json = json.loads(res_body)
                result_text = res_json['choices'][0]['message']['content'].strip()
            
            # 容错：去除大模型可能自带的 markdown 标记
            if result_text.startswith("```json"):
                result_text = result_text[7:-3]
            elif result_text.startswith("```"):
                result_text = result_text[3:-3]
                
            new_command = json.loads(result_text)
            
            # 4. 更新全局指令
            with command_lock:
                global_command["target_x"] = float(new_command.get("target_x", 375.0))
                global_command["target_y"] = float(new_command.get("target_y", 800.0))
                global_command["fire"] = int(new_command.get("fire", 1))
                
            print(f"[LLM Thread] 战术更新耗时 {time.time()-start_time:.2f}s, 新指令: {global_command}")

        except urllib.error.URLError as e:
            print(f"[LLM Thread] 网络请求失败: {e}")
        except json.JSONDecodeError:
            print(f"[LLM Thread] JSON 解析失败，大模型返回了非预期格式: {result_text}")
        except Exception as e:
            print(f"[LLM Thread] 发生未知错误: {e}")
            
        # 思考冷却时间（防止 API 计费爆炸）
        time.sleep(1.5)

def main():
    """主函数：UDP 服务器循环（反射神经）"""
    global global_game_state, global_command

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((LOCAL_IP, LOCAL_PORT))
    
    # 开启后台大模型请求线程
    llm_thread = threading.Thread(target=llm_worker_thread, daemon=True)
    llm_thread.start()

    print(f"[UDP Server] 启动成功，监听 {LOCAL_IP}:{LOCAL_PORT}")
    cpp_addr = ("127.0.0.1", CPP_PORT)

    try:
        while True:
            sock.settimeout(0.1)
            try:
                data, addr = sock.recvfrom(8192)
                cpp_addr = addr
                
                new_state = json.loads(data.decode('utf-8'))
                with state_lock:
                    global_game_state = new_state
                    
            except socket.timeout:
                pass 
            except ConnectionResetError:
                # 捕获 WinError 10054 报错。
                pass
            except json.JSONDecodeError:
                pass

            with command_lock:
                cmd_copy = global_command.copy()
                
            response = json.dumps(cmd_copy)
            
            try:
                sock.sendto(response.encode('utf-8'), cpp_addr)
            except ConnectionResetError:
                pass

            time.sleep(0.05) 

    except KeyboardInterrupt:
        print("\n[Server] 收到中断信号，正在关闭...")
    finally:
        sock.close()

if __name__ == "__main__":
    main()