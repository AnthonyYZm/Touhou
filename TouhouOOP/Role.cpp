#include "Role.h"

// 带 Alpha 通道的 putimage（支持裁剪）

void Role::putimagePNG(int x, int y, int w, int h, IMAGE* srcImg, int sx, int sy)
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