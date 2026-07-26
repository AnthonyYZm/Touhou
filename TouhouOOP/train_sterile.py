# -*- coding: utf-8 -*-
try:
    from stable_baselines3 import PPO
    from stable_baselines3.common.env_checker import check_env
except ModuleNotFoundError as exc:
    raise SystemExit(
        "缺少强化学习训练依赖。请先在项目根目录运行：python -m pip install -r requirements.txt"
    ) from exc
from touhou_env import TouhouSterileEnv

def main():
    print("=== 东方 RL 无菌室训练启动 ===")
    
    # 1. 初始化我们自己写的 Gymnasium 环境
    env = TouhouSterileEnv()
    
    # 2. 检查环境接口是否符合 Gymnasium 规范 (这步很重要，能帮你排错)
    print("正在检查环境接口兼容性...")
    check_env(env)
    print("环境接口检查通过！")
    
    # 3. 创建 PPO 模型
    # MlpPolicy: 多层感知机神经网络
    # tensorboard_log: 用于后续查看训练曲线
    model = PPO(
        "MlpPolicy", 
        env, 
        verbose=1, 
        learning_rate=0.0003,
        n_steps=2048,           # 每次收集多少帧数据更新一次神经网络
        batch_size=64,
        tensorboard_log="./touhou_tensorboard/"
    )
    
    # 4. 开始训练
    # 假设我们先训练 50 万步试试水
    total_timesteps = 500_000
    print(f"开始训练，总步数: {total_timesteps}，请确保 C++ 游戏已进入主菜单的【RL 训练】模式...")
    
    try:
        model.learn(total_timesteps=total_timesteps)
    except KeyboardInterrupt:
        print("\n收到中断信号，提前保存模型...")
        
    # 5. 保存训练好的模型
    save_path = "touhou_reimu_sterile_v1"
    model.save(save_path)
    print(f"训练完成！模型已保存为 {save_path}.zip")
    
    env.close()

if __name__ == "__main__":
    main()
