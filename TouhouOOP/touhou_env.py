# -*- coding: utf-8 -*-
import gymnasium as gym
from gymnasium import spaces
import numpy as np
import socket
import json


class TouhouSterileEnv(gym.Env):
    """
    Touhou RL sterile-room training environment.

    Python is the Gymnasium client. The C++ game must enter RL training mode,
    bind UDP 9090, and respond to reset/step requests with:
        {"ok": true, "state": {...}, "reward": 0.1, "done": false}
    """

    metadata = {"render_modes": []}

    def __init__(self, local_ip="127.0.0.1", local_port=8080, cpp_port=9090, timeout=2.0):
        super().__init__()

        # 动作维持最小离散控制：不动、上、下、左、右，先把训练闭环跑稳。
        self.action_space = spaces.Discrete(5)
        # 观测协议对应 C++ RLState：自机 2 维、Boss 相对位置 2 维、最近 10 颗弹幕 20 维。
        self.observation_space = spaces.Box(
            low=-2.0, high=2.0, shape=(24,), dtype=np.float32
        )

        # Python 固定绑定 8080，C++ 训练模式固定监听 9090；不要同时运行 ai_server.py。
        self.cpp_addr = (local_ip, cpp_port)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            self.sock.bind((local_ip, local_port))
        except OSError as exc:
            raise RuntimeError(
                f"无法绑定 Python UDP 端口 {local_ip}:{local_port}。"
                "请确认 ai_server.py 或其他训练进程没有占用 8080。"
            ) from exc
        self.sock.settimeout(timeout)

    def _parse_state(self, state_dict):
        # C++ 回包缺失某些字段时用 0 填充，保持 PPO 输入维度始终固定。
        obs = np.zeros(24, dtype=np.float32)
        obs[0] = state_dict.get("heroX", 0.0)
        obs[1] = state_dict.get("heroY", 0.0)
        obs[2] = state_dict.get("bossRelativeX", 0.0)
        obs[3] = state_dict.get("bossRelativeY", 0.0)

        bullets_x = state_dict.get("bulletRelativeX", [])
        bullets_y = state_dict.get("bulletRelativeY", [])

        for i in range(min(10, len(bullets_x), len(bullets_y))):
            obs[4 + i * 2] = bullets_x[i]
            obs[5 + i * 2] = bullets_y[i]

        return obs

    def _request_cpp(self, payload):
        # 每个 Gym reset/step 都是一次同步 UDP 请求；没收到响应就显式失败，避免假训练。
        self.sock.sendto(json.dumps(payload).encode("utf-8"), self.cpp_addr)

        try:
            data, _ = self.sock.recvfrom(8192)
        except socket.timeout as exc:
            raise RuntimeError(
                "等待 C++ RL 训练服务响应超时。请先启动游戏，进入主菜单的“RL 训练”，"
                "确认 UDP 9090 正在监听后再运行训练脚本。"
            ) from exc

        try:
            response = json.loads(data.decode("utf-8"))
        except json.JSONDecodeError as exc:
            raise RuntimeError(f"C++ 返回了非 JSON 数据: {data!r}") from exc

        if not response.get("ok", False):
            raise RuntimeError(f"C++ RL 训练服务返回错误: {response.get('error', response)}")

        return response

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)

        response = self._request_cpp({"command": "reset", "difficultyLevel": 1})
        obs = self._parse_state(response.get("state", {}))
        return obs, {}

    def step(self, action):
        response = self._request_cpp({"command": "step", "action": int(action)})

        obs = self._parse_state(response.get("state", {}))
        reward = float(response.get("reward", 0.0))
        done = bool(response.get("done", False))

        terminated = done and reward < 0
        truncated = done and reward >= 0
        return obs, reward, terminated, truncated, {}

    def close(self):
        self.sock.close()
