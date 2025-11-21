#pragma once
#include "Library.h"

class Role {
public:

	float x, y;
	int hp;
	bool alive, fire;

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
	}

	virtual void draw() = 0;
	
	//virtual void move() = 0;
};

/*Handle Type*/
enum class eType : int {
    normal = 0,
    elf = 1
};

enum class bType : int {
    down_st = 0,
    windmill_st = 1
};


/*Put images*/

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
