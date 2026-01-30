#pragma once
#include "Library.h"

// 弹幕任务结构体 
struct BarrageTask {
	int type;           // 弹幕类型 (对应 bType 枚举转 int)
	DWORD lastTime;     // 上一次发射的时间
	int interval;       // 发射间隔 (ms)
	float speed;        // 速度
	float acc;			// 加速度 
	float omega;        // 角速度 或 半径参数
	int num;            // 数量
	int r;              // 半径 (用于wheel等)
	int dir;            // 方向 (用于straightMill等)

	int burstCount;    // 连发次数 
	int burstInterval; // 连发间隔
	int currentBurst; // 当前连发计数	
	DWORD lastBurstTime; // 上一发时间

	// 构造函数
    BarrageTask(int _type, int _interval, float _speed, float _omega, int _num, int _r = 0, int _dir = 1,
        int _burstCount = 1, int _burstInterval = 0, float _speedDec = 0.0f) {
        type = _type;
        interval = _interval;
        speed = _speed;
        omega = _omega;
        num = _num;
        r = _r;
        dir = _dir;
        lastTime = 0;

        burstCount = _burstCount;
        burstInterval = _burstInterval;
        acc = _speedDec;
        currentBurst = 0;
        lastBurstTime = 0;
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

inline void putimagePNG(int x, int y, int w, int h, IMAGE* srcImg, int sx, int sy)
{
    DWORD* dst = GetImageBuffer();
    DWORD* src = GetImageBuffer(srcImg);

    int srcWidth = srcImg->getwidth();
    int dstWidth = getwidth();
    int dstHeight = getheight();

    for (int i = 0; i < h; i++) {
        if (y + i < 0 || y + i >= dstHeight) continue;
        for (int j = 0; j < w; j++) {
            if (x + j < 0 || x + j >= dstWidth) continue;

            DWORD sc = src[(sy + i) * srcWidth + (sx + j)];
            BYTE sa = (sc >> 24) & 0xFF;  // Alpha
            if (sa == 0) continue;

            BYTE sb = (sc >> 16) & 0xFF;  // Blue
            BYTE sg = (sc >> 8) & 0xFF;   // Green
            BYTE sr = sc & 0xFF;          // Red

            DWORD& dc = dst[(y + i) * dstWidth + (x + j)];
            BYTE db = (dc >> 16) & 0xFF;
            BYTE dg = (dc >> 8) & 0xFF;
            BYTE dr = dc & 0xFF;

            BYTE r = (sr * sa + dr * (255 - sa)) / 255;
            BYTE g = (sg * sa + dg * (255 - sa)) / 255;
            BYTE b = (sb * sa + db * (255 - sa)) / 255;

            dc = RGB(r, g, b);
        }
    }
}

inline void putimagePNG(int x, int y, int srcW, int srcH, IMAGE* srcImg, int sx, int sy, int dstW, int dstH)
{
    DWORD* dst = GetImageBuffer();
    DWORD* src = GetImageBuffer(srcImg);

    int srcTotalWidth = srcImg->getwidth();
    int srcTotalHeight = srcImg->getheight();
    int dstTotalWidth = getwidth();
    int dstTotalHeight = getheight();

    // 定义允许绘制的裁剪区域 (Clip Region)
    // 如果你想让特效只显示在游戏框内：
    int clipLeft = LeftEdge;
    int clipTop = TopEdge;
    int clipRight = LeftEdge + WIDTH;
    int clipBottom = TopEdge + HEIGHT;

    for (int i = 0; i < dstH; i++) {
        int screenY = y + i;
        // 1. 屏幕物理边界检查
        if (screenY < 0 || screenY >= dstTotalHeight) continue;
        // 2. 游戏逻辑边界检查 (Clip)
        if (screenY < clipTop || screenY >= clipBottom) continue;

        int sourceY = sy + (i * srcH) / dstH;
        if (sourceY >= srcTotalHeight) continue;

        for (int j = 0; j < dstW; j++) {
            int screenX = x + j;
            // 1. 屏幕物理边界检查
            if (screenX < 0 || screenX >= dstTotalWidth) continue;
            // 2. 游戏逻辑边界检查 (Clip)
            if (screenX < clipLeft || screenX >= clipRight) continue;

            int sourceX = sx + (j * srcW) / dstW;
            if (sourceX >= srcTotalWidth) continue;

            DWORD sc = src[sourceY * srcTotalWidth + sourceX];
            BYTE sa = (sc >> 24) & 0xFF;
            if (sa == 0) continue;

            BYTE sb = (sc >> 16) & 0xFF;
            BYTE sg = (sc >> 8) & 0xFF;
            BYTE sr = sc & 0xFF;

            DWORD& dc = dst[screenY * dstTotalWidth + screenX];
            BYTE db = (dc >> 16) & 0xFF;
            BYTE dg = (dc >> 8) & 0xFF;
            BYTE dr = dc & 0xFF;

            BYTE r = (sr * sa + dr * (255 - sa)) / 255;
            BYTE g = (sg * sa + dg * (255 - sa)) / 255;
            BYTE b = (sb * sa + db * (255 - sa)) / 255;

            dc = RGB(r, g, b);
        }
    }
}
