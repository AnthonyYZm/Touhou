#pragma once
#include "Library.h"

// Handle Type
enum class eType : int {
    normal = 0,
    elf = 1
};

enum class bType : int {
    down_st = 0,
    windmill_st = 1,
    firework = 2,
	circle_mill = 3,
    wheel = 4
};

// 弹幕任务结构体 
struct BarrageTask {
	int type;           // 弹幕类型 (对应 bType 枚举转 int)
	DWORD lastTime;     // 上一次发射的时间
	int interval;       // 发射间隔 (ms)

	float speed;        // 速度
	float omega;        // 角速度 或 半径参数
	int num;            // 数量
	int r;              // 半径 (用于wheel等)
	int dir;            // 方向 (用于straightMill等)

	// 构造函数
	BarrageTask(int _type, int _interval, float _speed, float _omega, int _num, int _r = 0, int _dir = 1) {
		type = _type;
		interval = _interval;
		speed = _speed;
		omega = _omega;
		num = _num;
		r = _r;
		dir = _dir;
		lastTime = 0;
	}
};


class Role {
public:

	float x, y;
	int hp;
	bool alive, fire, lock;
	eType type;

	Role(float _x, float _y, int _hp) {
		x = _x;
		y = _y;
		hp = _hp;
		alive = true;
		fire = false;
	}

	Role(float _x, float _y) {
		x = _x;
		y = _y;
        alive = true;
        fire = false;
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
