#pragma once
#pragma execution_character_set("utf-8")
#include "Enemy.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

// 移动策略库。MoveStrategy 的 timer 参数统一表示“敌人出生后经过的毫秒数”。
namespace Moves {

    struct Point {
        float x;
        float y;
    };

    inline float Clamp01(float value) {
        return (std::max)(0.0f, (std::min)(1.0f, value));
    }

    inline float SmoothStep(float value) {
        const float t = Clamp01(value);
        return t * t * (3.0f - 2.0f * t);
    }

    inline float EaseInOutSine(float value) {
        const float t = Clamp01(value);
        return -(cos(PI * t) - 1.0f) * 0.5f;
    }

    inline float Lerp(float from, float to, float t) {
        return from + (to - from) * Clamp01(t);
    }

    inline Point Lerp(Point from, Point to, float t) {
        return { Lerp(from.x, to.x, t), Lerp(from.y, to.y, t) };
    }

    inline Point CubicBezier(Point p0, Point p1, Point p2, Point p3, float t) {
        const float u = Clamp01(t);
        const float inv = 1.0f - u;
        const float w0 = inv * inv * inv;
        const float w1 = 3.0f * inv * inv * u;
        const float w2 = 3.0f * inv * u * u;
        const float w3 = u * u * u;
        return { w0 * p0.x + w1 * p1.x + w2 * p2.x + w3 * p3.x,
                 w0 * p0.y + w1 * p1.y + w2 * p2.y + w3 * p3.y };
    }

    inline Point Normalize(Point vector, Point fallback = { 0.0f, -1.0f }) {
        const float length = sqrt(vector.x * vector.x + vector.y * vector.y);
        if (length < 0.0001f) return fallback;
        return { vector.x / length, vector.y / length };
    }

    // 旧接口：vx/vy 为像素/帧，保留给既有关卡使用。
    static MoveStrategy Linear(float vxPerFrame, float vyPerFrame) {
        return [=](Enemy* enemy, int timerMs) {
            enemy->motionState = MotionState::MOVING;
            enemy->fire = true;
            enemy->x += vxPerFrame;
            enemy->y += vyPerFrame;
        };
    }

    // 旧接口：speedY 为像素/帧，保留原行为；新轨迹请使用 ZigzagDive/BezierPass。
    static MoveStrategy SineWave(float centerX, float amplitude, float freq, float speedYPerFrame) {
        return [=](Enemy* enemy, int timerMs) {
            enemy->motionState = MotionState::MOVING;
            enemy->fire = true;
            enemy->y += speedYPerFrame;
            enemy->x = centerX + amplitude * sin(timerMs * freq * 0.001f);
        };
    }

    // 旧接口：speed 为像素/帧。
    static MoveStrategy Hover(float targetY, float speedPerFrame) {
        return [=](Enemy* enemy, int timerMs) {
            if (enemy->y < targetY) {
                enemy->motionState = MotionState::ENTERING;
                enemy->fire = false;
                enemy->y += speedPerFrame;
            }
            else {
                enemy->y = targetY;
                enemy->motionState = MotionState::HOLDING;
                enemy->fire = true;
            }
        };
    }

    // 旧接口：speed 为像素/帧。默认 Boss 进入上方中央，不再飞至玩家活动区中央。
    static MoveStrategy bossEnter(float speedPerFrame) {
        return [=](Enemy* enemy, int timerMs) {
            const float targetX = LeftEdge + WIDTH * 0.5f;
            const float targetY = TopEdge + 100.0f;
            const float dx = targetX - enemy->x;
            const float dy = targetY - enemy->y;
            const float distance = sqrt(dx * dx + dy * dy);
            if (distance <= (std::max)(0.1f, speedPerFrame)) {
                enemy->x = targetX;
                enemy->y = targetY;
                enemy->motionState = MotionState::HOLDING;
                enemy->fire = true;
            }
            else {
                enemy->motionState = MotionState::ENTERING;
                enemy->fire = false;
                enemy->x += speedPerFrame * dx / distance;
                enemy->y += speedPerFrame * dy / distance;
            }
        };
    }

    // 旧接口：speed 为像素/帧。
    static MoveStrategy shuttle(float speedPerFrame, int dir) {
        return [=](Enemy* enemy, int timerMs) {
            enemy->motionState = MotionState::MOVING;
            enemy->fire = true;
            enemy->x += (dir == 0) ? -speedPerFrame : speedPerFrame;
        };
    }

    static MoveStrategy Stay() {
        return [](Enemy* enemy, int timerMs) {
            enemy->motionState = MotionState::HOLDING;
            enemy->fire = true;
        };
    }

    // 旧接口：interval/duration 为毫秒，speedX/Y 为像素/帧。
    static MoveStrategy StepLeftUp(int intervalMs, int durationMs, float speedXPerFrame, float speedYPerFrame) {
        return [=](Enemy* enemy, int timerMs) {
            const int safeInterval = (std::max)(1, intervalMs);
            const int t = timerMs % safeInterval;
            const int moveCount = timerMs / safeInterval;
            enemy->motionState = MotionState::MOVING;
            enemy->motionSegment = moveCount % 3;
            enemy->fire = true;
            if (t >= (std::max)(0, durationMs)) return;
            switch (enemy->motionSegment) {
            case 0: enemy->x -= speedXPerFrame; enemy->y -= speedYPerFrame; break;
            case 1: enemy->y += speedYPerFrame * 2.0f; break;
            default: enemy->x += speedXPerFrame; enemy->y -= speedYPerFrame; break;
            }
        };
    }

    // 旧接口：speed 为像素/帧。
    static MoveStrategy MoveTo(float targetX, float targetY, float speedPerFrame) {
        return [=](Enemy* enemy, int timerMs) {
            const float dx = targetX - enemy->x;
            const float dy = targetY - enemy->y;
            const float distance = sqrt(dx * dx + dy * dy);
            if (distance <= (std::max)(0.1f, speedPerFrame)) {
                enemy->x = targetX;
                enemy->y = targetY;
                enemy->motionState = MotionState::HOLDING;
                enemy->fire = true;
            }
            else {
                enemy->motionState = MotionState::MOVING;
                enemy->fire = false;
                enemy->x += speedPerFrame * dx / distance;
                enemy->y += speedPerFrame * dy / distance;
            }
        };
    }

    // 新接口均使用毫秒 timer 和像素/秒。
    static MoveStrategy EnterHoldExit(Point start, Point hold, int enterDurationMs, int holdDurationMs,
        float exitSpeedPxPerSec, bool fireOnlyWhileHolding = true) {
        return [=](Enemy* enemy, int timerMs) {
            const int enterMs = (std::max)(1, enterDurationMs);
            const int holdMs = (std::max)(0, holdDurationMs);
            if (timerMs < enterMs) {
                const float t = SmoothStep((float)timerMs / enterMs);
                const Point position = Lerp(start, hold, t);
                enemy->x = position.x; enemy->y = position.y;
                enemy->motionState = MotionState::ENTERING;
                enemy->fire = !fireOnlyWhileHolding;
                return;
            }
            if (timerMs < enterMs + holdMs) {
                enemy->x = hold.x; enemy->y = hold.y;
                enemy->motionState = MotionState::HOLDING;
                enemy->fire = true;
                return;
            }
            const float seconds = (timerMs - enterMs - holdMs) * 0.001f;
            enemy->x = hold.x;
            enemy->y = hold.y - (std::max)(20.0f, exitSpeedPxPerSec) * seconds;
            enemy->motionState = MotionState::EXITING;
            enemy->fire = false;
        };
    }

    static MoveStrategy BezierPass(Point p0, Point p1, Point p2, Point p3, int durationMs,
        bool fireDuringMove = false, float fireStart = 0.0f, float fireEnd = 1.0f, float exitSpeedPxPerSec = 190.0f) {
        return [=](Enemy* enemy, int timerMs) {
            const int safeDuration = (std::max)(1, durationMs);
            const float begin = Clamp01((std::min)(fireStart, fireEnd));
            const float end = Clamp01((std::max)(fireStart, fireEnd));
            if (timerMs < safeDuration) {
                const float t = (float)timerMs / safeDuration;
                const Point position = CubicBezier(p0, p1, p2, p3, t);
                enemy->x = position.x; enemy->y = position.y;
                enemy->motionState = MotionState::MOVING;
                enemy->fire = fireDuringMove && t >= begin && t <= end;
                return;
            }
            const Point tangent = Normalize({ p3.x - p2.x, p3.y - p2.y });
            const float seconds = (timerMs - safeDuration) * 0.001f;
            enemy->x = p3.x + tangent.x * exitSpeedPxPerSec * seconds;
            enemy->y = p3.y + tangent.y * exitSpeedPxPerSec * seconds;
            enemy->motionState = MotionState::EXITING;
            enemy->fire = false;
        };
    }

    static MoveStrategy ZigzagDive(float centerX, float startY, float amplitude, int periodMs,
        float downwardSpeedPxPerSec, float initialPhase = 0.0f) {
        return [=](Enemy* enemy, int timerMs) {
            const int safePeriod = (std::max)(1, periodMs);
            const float phase = 2.0f * PI * ((float)timerMs / safePeriod) + initialPhase;
            const float triangle = 2.0f / PI * asinf(sinf(phase));
            enemy->x = centerX + amplitude * triangle;
            enemy->y = startY + downwardSpeedPxPerSec * timerMs * 0.001f;
            enemy->motionState = MotionState::MOVING;
            enemy->fire = true;
        };
    }

    static MoveStrategy OrbitThenExit(Point center, float radiusX, float radiusY, int orbitDurationMs,
        float startAngle, bool clockwise, float exitSpeedPxPerSec = 180.0f) {
        return [=](Enemy* enemy, int timerMs) {
            const int duration = (std::max)(1, orbitDurationMs);
            const float direction = clockwise ? 1.0f : -1.0f;
            const float clampedX = (std::max)(1.0f, fabs(radiusX));
            const float clampedY = (std::max)(1.0f, fabs(radiusY));
            const float progress = (std::min)(1.0f, (float)timerMs / duration);
            const float angle = startAngle + direction * 2.0f * PI * progress;
            if (timerMs < duration) {
                enemy->x = center.x + clampedX * cos(angle);
                enemy->y = center.y + clampedY * sin(angle);
                enemy->motionState = MotionState::MOVING;
                enemy->fire = false;
                return;
            }
            const Point tangent = Normalize({ -clampedX * sin(angle) * direction, clampedY * cos(angle) * direction });
            const float seconds = (timerMs - duration) * 0.001f;
            enemy->x = center.x + clampedX * cos(angle) + tangent.x * exitSpeedPxPerSec * seconds;
            enemy->y = center.y + clampedY * sin(angle) + tangent.y * exitSpeedPxPerSec * seconds;
            enemy->motionState = MotionState::EXITING;
            enemy->fire = false;
        };
    }

    static MoveStrategy FigureEight(Point center, float radiusX, float radiusY, int periodMs,
        float startAngle = 0.0f, int activeDurationMs = 0, float exitSpeedPxPerSec = 180.0f) {
        return [=](Enemy* enemy, int timerMs) {
            const int safePeriod = (std::max)(1, periodMs);
            const int activeMs = (std::max)(0, activeDurationMs);
            const int movementTime = (activeMs > 0) ? (std::min)(timerMs, activeMs) : timerMs;
            // 分段编号只描述运动节拍，不依赖具体位置；弹幕任务可据此交替启用。
            enemy->motionSegment = (movementTime / 3000) % 2;
            const float angle = startAngle + 2.0f * PI * movementTime / safePeriod;
            const float rx = (std::max)(1.0f, fabs(radiusX));
            const float ry = (std::max)(1.0f, fabs(radiusY));
            Point position = { center.x + rx * sin(angle), center.y + ry * sin(2.0f * angle) };
            if (activeMs == 0 || timerMs < activeMs) {
                enemy->x = position.x; enemy->y = position.y;
                enemy->motionState = MotionState::MOVING;
                enemy->fire = true;
                return;
            }
            const Point tangent = Normalize({ rx * cos(angle), 2.0f * ry * cos(2.0f * angle) });
            const float seconds = (timerMs - activeMs) * 0.001f;
            enemy->x = position.x + tangent.x * exitSpeedPxPerSec * seconds;
            enemy->y = position.y + tangent.y * exitSpeedPxPerSec * seconds;
            enemy->motionState = MotionState::EXITING;
            enemy->fire = false;
        };
    }

    static MoveStrategy EllipsePatrol(Point center, float radiusX, float radiusY, int periodMs,
        float startAngle = 0.0f, bool clockwise = true) {
        return [=](Enemy* enemy, int timerMs) {
            const int safePeriod = (std::max)(1, periodMs);
            const float direction = clockwise ? 1.0f : -1.0f;
            const float angle = startAngle + direction * 2.0f * PI * timerMs / safePeriod;
            enemy->x = center.x + (std::max)(1.0f, fabs(radiusX)) * cos(angle);
            enemy->y = center.y + (std::max)(1.0f, fabs(radiusY)) * sin(angle);
            enemy->motionState = MotionState::MOVING;
            enemy->fire = true;
        };
    }

    static MoveStrategy BurstWaypointPatrol(std::vector<Point> waypoints, int moveDurationMs,
        int holdDurationMs, bool loop) {
        return [waypoints, moveDurationMs, holdDurationMs, loop](Enemy* enemy, int timerMs) {
            if (waypoints.empty()) {
                enemy->motionState = MotionState::HOLDING;
                enemy->fire = false;
                return;
            }
            if (waypoints.size() == 1) {
                enemy->x = waypoints.front().x; enemy->y = waypoints.front().y;
                enemy->motionState = MotionState::HOLDING;
                enemy->motionSegment = 0;
                enemy->fire = true;
                return;
            }
            const int moveMs = (std::max)(1, moveDurationMs);
            const int holdMs = (std::max)(0, holdDurationMs);
            const int cycleMs = moveMs + holdMs;
            const int segmentCount = loop ? (int)waypoints.size() : (int)waypoints.size() - 1;
            const int totalMs = segmentCount * cycleMs;
            if (!loop && timerMs >= totalMs) {
                enemy->x = waypoints.back().x; enemy->y = waypoints.back().y;
                enemy->motionState = MotionState::HOLDING;
                enemy->motionSegment = segmentCount;
                enemy->fire = true;
                return;
            }
            const int safeTimer = (std::max)(0, timerMs);
            const int segment = (safeTimer / cycleMs) % segmentCount;
            const int localTime = safeTimer % cycleMs;
            const Point from = waypoints[segment];
            const Point to = waypoints[(segment + 1) % waypoints.size()];
            enemy->motionSegment = segment;
            if (localTime < moveMs) {
                const Point position = Lerp(from, to, SmoothStep((float)localTime / moveMs));
                enemy->x = position.x; enemy->y = position.y;
                enemy->motionState = MotionState::MOVING;
                enemy->fire = false;
            }
            else {
                enemy->x = to.x; enemy->y = to.y;
                enemy->motionState = MotionState::HOLDING;
                enemy->fire = true;
            }
        };
    }

    static MoveStrategy FlowerPath(Point center, float radiusX, float radiusY, int periodMs,
        int petals = 5, float startAngle = 0.0f, int activeDurationMs = 0, float exitSpeedPxPerSec = 170.0f) {
        return [=](Enemy* enemy, int timerMs) {
            const int safePeriod = (std::max)(1, periodMs);
            const int safePetals = (std::max)(1, petals);
            const int activeMs = (std::max)(0, activeDurationMs);
            const int movementTime = (activeMs > 0) ? (std::min)(timerMs, activeMs) : timerMs;
            const float angle = startAngle + 2.0f * PI * movementTime / safePeriod;
            const float petalScale = 0.55f + 0.45f * sin(safePetals * angle);
            Point position = { center.x + radiusX * cos(angle) * petalScale,
                               center.y + radiusY * sin(angle) * petalScale };
            if (activeMs == 0 || timerMs < activeMs) {
                enemy->x = position.x; enemy->y = position.y;
                enemy->motionState = MotionState::MOVING;
                enemy->fire = true;
                return;
            }
            const Point tangent = Normalize({ -radiusX * sin(angle), radiusY * cos(angle) });
            const float seconds = (timerMs - activeMs) * 0.001f;
            enemy->x = position.x + tangent.x * exitSpeedPxPerSec * seconds;
            enemy->y = position.y + tangent.y * exitSpeedPxPerSec * seconds;
            enemy->motionState = MotionState::EXITING;
            enemy->fire = false;
        };
    }
}
