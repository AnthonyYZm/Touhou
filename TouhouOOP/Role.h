#pragma once
#pragma execution_character_set("utf-8")
#include "Library.h"

enum class EmitterAnchor {
    ENEMY_CENTER,
    ENEMY_OFFSET,
    SCREEN_ABSOLUTE
};

// 弹幕任务结构体 
struct BarrageTask {
    int type;            // 弹幕类型 
    DWORD lastTime;      // 上一次发射的时间
    int interval;        // 发射间隔 
    float speed;         // 速度
    float omega;         // 角速度 或 半径参数
    int num;             // 数量
    int r;               // 半径 (用于wheel等)
    int dir;             // 方向 (用于straightMill等)
    int x0, y0;          // 发射位置
    float acc;		   	 // 加速度 
    int burstCount;      // 连发次数 
    int burstInterval;   // 连发间隔
    int currentBurst;    // 当前连发计数
    float currentAngle;  // 当前角度 (用于旋转类弹幕)
    DWORD lastBurstTime; // 上一发时间
    int style;           // 弹幕样式
    int startDelay;      // 阶段开始后的首次发射延迟，用于错开不同弹幕周期
    unsigned int jitterState; // 风车错位用的任务私有伪随机状态，禁止不同任务共享
    EmitterAnchor anchorMode; // 发射点锚定方式，默认随敌人中心移动
    int offsetX;
    int offsetY;
    bool requireHolding;      // true 时仅在移动策略报告 HOLDING 后发射
    int requiredMotionSegment; // -1 表示任意路点段
    bool invertDirectionBySegment; // 奇数路点段反转旋转方向

    BarrageTask(int _type, int _interval, float _speed, float _omega, int _num, int _r = 0, 
        int _dir = 1, int _x = 0, int _y = 0, float _acc = 0.0f, 
        int _burstCount = 1, int _burstInterval = 0, int _style = 0) {
        type = _type;
        interval = _interval;
        speed = _speed;
        omega = _omega;
        num = _num;
        r = _r;
        dir = _dir;
        x0 = _x; y0 = _y;
        lastTime = 0;
        burstCount = _burstCount;
        burstInterval = _burstInterval;
        acc = _acc;
        currentBurst = 0;
        lastBurstTime = 0;
		currentAngle = 0.0f;
		style = _style;
        startDelay = 0;
        // 由任务配置生成稳定但彼此独立的种子；不使用函数 static 或全局 rand 状态。
        jitterState = 2166136261u;
        jitterState = (jitterState ^ (unsigned int)_type) * 16777619u;
        jitterState = (jitterState ^ (unsigned int)_x) * 16777619u;
        jitterState = (jitterState ^ (unsigned int)_y) * 16777619u;
        jitterState = (jitterState ^ (unsigned int)(_dir + 3)) * 16777619u;
        jitterState = (jitterState ^ (unsigned int)(static_cast<int>(_style) + 7)) * 16777619u;
        if (jitterState == 0) jitterState = 1;
        anchorMode = EmitterAnchor::ENEMY_CENTER;
        offsetX = 0;
        offsetY = 0;
        requireHolding = false;
        requiredMotionSegment = -1;
        invertDirectionBySegment = false;
    }
};


class Role {
public:

	float x, y;
	int hp;
	bool alive, fire, lock;

	Role(float _x, float _y, int _hp) {
		x = _x;
		y = _y;
		hp = _hp;
		alive = true;
		fire = false;
		lock = false;
	}

	Role(float _x, float _y) {
		x = _x;
		y = _y;
        alive = true;
        fire = false;
		lock = false;
	}

    virtual ~Role() {}
	virtual void draw() = 0;
    virtual void move() = 0;
};


// Put images

inline void putimagePNG(int x, int y, int srcW, int srcH, IMAGE* srcImg, int sx, int sy, int dstW, int dstH)
{
    int clipLeft = LeftEdge;
    int clipTop = TopEdge;
    int clipRight = LeftEdge + WIDTH;
    int clipBottom = TopEdge + HEIGHT;

    if (x >= clipRight || y >= clipBottom || x + dstW <= clipLeft || y + dstH <= clipTop) {
        return;
    }

    HDC hDC = GetImageHDC(NULL);
    HDC hSrc = GetImageHDC(srcImg);

    BLENDFUNCTION bf = { 0 };
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;

    // 绝大多数弹幕完整位于游戏区：直接绘制，避免每颗都 SaveDC/裁剪/RestoreDC。
    if (x >= clipLeft && y >= clipTop && x + dstW <= clipRight && y + dstH <= clipBottom) {
        AlphaBlend(hDC, x, y, dstW, dstH, hSrc, sx, sy, srcW, srcH, bf);
        return;
    }

    // 只有贴边对象才需要临时裁剪，保持不会画入右侧 UI 区域的原有效果。
    int savedDC = SaveDC(hDC);
    IntersectClipRect(hDC, clipLeft, clipTop, clipRight, clipBottom);
    AlphaBlend(
        hDC, x, y, dstW, dstH,
        hSrc, sx, sy, srcW, srcH,
        bf
    );

    RestoreDC(hDC, savedDC);
}
