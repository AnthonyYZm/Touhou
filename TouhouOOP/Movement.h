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
            if (t < duration && e->y > TopEdge + 50) {
                if (e->x > 20) e->x -= speedX;
                if (e->y > 20) e->y -= speedY;
            }
            else {
                e->y = e->y;
                e->x = e->x;
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