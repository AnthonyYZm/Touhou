#pragma once
#include "Enemy.h"
#include <cmath>
#include <functional>

// 移动策略库
namespace Moves {

    // 1. 直线移动 (参数：X轴速度，Y轴速度)
    static MoveStrategy Linear(float vx, float vy) {
        return [=](Enemy* e, int t) {
            e->x += vx;
            e->y += vy;
            };
    }

    // 2. 正弦波移动 (参数：中心X，振幅，频率，下落速度)
    // 效果：一边下落一边左右摇摆
    static MoveStrategy SineWave(float centerX, float amplitude, float freq, float speedY) {
        return [=](Enemy* e, int t) {
            e->y += speedY; // Y轴匀速
            // X轴震荡：sin(时间 * 频率)
            e->x = centerX + amplitude * sin(t * freq * 0.001f);
            };
    }

    // 3. 悬停 (参数：目标Y，进场速度)
    // 效果：飞到指定Y高度后停住
    static MoveStrategy Hover(float targetY, float speed) {
        return [=](Enemy* e, int t) {
            if (e->y < targetY) {
                e->y += speed;
            }
            // 到达后不动，或者可以在这里加微小的浮动
            };
    }

    static MoveStrategy bossEnter(float speed) {
        return [=](Enemy* e, int t) {
            float theta = atan2(CentralY - e->y, CentralX - e->x);
            if (e->y < CentralY) {
                e->y += speed * sin(theta);
                e->x += speed * cos(theta);
            }
            };
	}

    static MoveStrategy shuttle(float speed) {
        return [=](Enemy* e, int t) {
            static int count = 0;
            e->x += speed;
            if (e->x > Right && count < 2) e->x = LeftEdge;
            if (e->x == CentralX) count++;
            };
	}

    // 4. 贝塞尔曲线或追踪逻辑可在此处扩展...
}