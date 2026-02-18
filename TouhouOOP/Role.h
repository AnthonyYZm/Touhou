#pragma once
#include "Library.h"

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

    HDC hDC = GetImageHDC(NULL);
    HDC hSrc = GetImageHDC(srcImg);

    int clipLeft = LeftEdge;
    int clipTop = TopEdge;
    int clipRight = LeftEdge + WIDTH;
    int clipBottom = TopEdge + HEIGHT;

    if (x >= clipRight || y >= clipBottom || x + dstW <= clipLeft || y + dstH <= clipTop) {
        return;
    }
    // GDI 裁剪 
    int savedDC = SaveDC(hDC);

    // 设置裁剪区域：告诉显卡，只允许在 (clipLeft, clipTop) 到 (clipRight, clipBottom) 之间画图
    IntersectClipRect(hDC, clipLeft, clipTop, clipRight, clipBottom);

    // 绘制 (AlphaBlend)
    BLENDFUNCTION bf = { 0 };
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;

    AlphaBlend(
        hDC, x, y, dstW, dstH,
        hSrc, sx, sy, srcW, srcH,
        bf
    );

    // 恢复状态
    RestoreDC(hDC, savedDC);
}
