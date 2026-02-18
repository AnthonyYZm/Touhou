# TouhouOOP - 东方 Project C++ 仿作项目

---

* 一个基于 **C++** 和 **EasyX** 图形库开发的东方 Project STG游戏。本项目实现了动态波次系统，可自由配置敌人。

---

## 技术栈
* **语言标准**: ISO C++17 。
* **图形庫**: [EasyX Graphics Library](https://easyx.cn/)。
* **核心架构**:
    * `Game`: 维护游戏主循环、碰撞检测及波次逻辑。
    * `EnemyManager`: 负责敌人的生成、掉落物管理及生命周期回收。
    * `Barrage`: 弹幕设计与渲染引擎。

---

##  游戏操作

| 按键 | 功能 |
| :--- | :--- |
| **↑ ↓ ← →** | 控制自机移动 |
| **Z** | 射击 |
| **X** | 释放符卡 (Bomb) |
| **Shift** | 低速模式，显示判定点 |

---

## 开发环境要求

* Windows 10/11
* 已安装 EasyX 图形库

 ---

## 阶段配置示例

```C++
// 创建一个普通敌人波次
SpawnEvent e1;
e1.type = eType::normal;
e1.count = 5;
e1.interval = 400;
e1.moveLogic = Moves::SineWave(CentralX, 50, 2.0f, 3.0f);
w1.events.push_back(e1);
waveQueue.push(w1);
```

 ---

## 许可证

本项目仅供学习与交流使用。游戏素材资源版权归属原作者（Team Shanghai Alice / ZUN）。
