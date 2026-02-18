#pragma once
#include "Enemy.h"
#include <cmath>
#include <functional>

// 移动策略库
namespace Moves {

    // 直线移动 
    static MoveStrategy Linear(float vx, float vy) {
        return [=](Enemy* e, int t) {
            e->x += vx;
            e->y += vy;
            };
    }

    // 2. 正弦波移动 (参数：中心X，振幅，频率，下落速度)
    static MoveStrategy SineWave(float centerX, float amplitude, float freq, float speedY) {
        return [=](Enemy* e, int t) {
            e->y += speedY; // Y轴匀速
            // X轴震荡：sin(时间 * 频率)
            e->x = centerX + amplitude * sin(t * freq * 0.001f);
            };
    }

    // 3. 悬停 
    static MoveStrategy Hover(float targetY, float speed) {
        return [=](Enemy* e, int t) {
            if (e->y < targetY) {
                e->y += speed;
            }
            };
    }

    static MoveStrategy bossEnter(float speed) {
        return [=](Enemy* e, int t) {
            float dx = CentralX - e->x;
            float dy = CentralY - e->y;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist <= speed) {
                e->x = CentralX;
                e->y = CentralY;
            }
            else {
                float theta = atan2(dy, dx);
                e->x += speed * cos(theta);
                e->y += speed * sin(theta);
            }
            };
    }

    static MoveStrategy shuttle(float speed, int dir) {
        return [=](Enemy* e, int t) {
            if (dir == 0) e->x -= speed;
			else e->x += speed;
            };
	}

    static MoveStrategy Stay() {
        return [=](Enemy* e, int t) {
            e->x = e->x;
            e->y = e->y;
            };
	}
   
	
    static MoveStrategy StepLeftUp(int interval, int duration, float speedX, float speedY) {
        return [=](Enemy* e, int timer) {
            int t = timer % interval;
            int moveCnt = timer / interval;
            if (t < duration) {
                // 根据周期数对 3 取模，决定当前周期的动作方向
                int actionType = moveCnt % 3;

                switch (actionType) {
                case 0: // 第一个周期：向左上移动
                    e->x -= speedX;
                    e->y -= speedY;
                    break;

                case 1: // 第二个周期：向下移动
                    e->y += speedY * 2; // 这里可以根据需要调整倍数
                    break;

                case 2: // 第三个周期：向右上移动
                    e->x += speedX;
                    e->y -= speedY;
                    break;
                }
            }
            };
    }

    static MoveStrategy MoveTo(float targetX, float targetY, float speed) {
        return [=](Enemy* e, int timer) {

            float dx = targetX - e->x;
            float dy = targetY - e->y;
            float dist = sqrt(dx * dx + dy * dy);

            if (dist < speed) {
                e->x = targetX;
                e->y = targetY;
            }
            else {
                float angle = atan2(dy, dx);
                e->x += speed * cos(angle);
                e->y += speed * sin(angle);
            }
            };
    }
}