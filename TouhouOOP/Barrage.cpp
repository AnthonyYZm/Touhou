#pragma execution_character_set("utf-8")
#include "Barrage.h"
#include "Game.h"

#include <cstring>

namespace {
    // 默认硬上限低于性能目标 1800；因此极端场景也不会长期逼近 2400。
    constexpr size_t kMaxLiveBarrages = 1800;
    constexpr DWORD kBarrageSoundInterval = 80;
    constexpr DWORD kForcedLockedLifetime = 18000;

    void playBarrageSoundThrottled() {
        static DWORD lastSoundTime = 0;
        const DWORD now = GetTickCount();
        if (lastSoundTime == 0 || (DWORD)(now - lastSoundTime) >= kBarrageSoundInterval) {
            Game::Audio.play(L"barrage");
            lastSoundTime = now;
        }
    }

    // 特效按波次抽样，避免高密度弹幕把大量 CREATE_BARRAGE 特效压入队列。
    void spawnBarrageEffectSampled(int index, float x, float y) {
        if (index == 0 || index % 8 == 0) {
            Game::Effects.spawn(EffectType::CREATE_BARRAGE, x, y);
        }
    }
}

int const Barrage::darkGreenWidth = 24;
int const Barrage::darkGreenHeight = 24;
int const Barrage::riceWidth = 9;
int const Barrage::riceHeight = 16;
int Barrage::wheelGroup = 1;
int Barrage::globalGroupID = 0;
IMAGE Barrage::imgBlue;
IMAGE Barrage::imgGreen;
IMAGE Barrage::imgRiceBlue;
IMAGE Barrage::imgRiceRed;
IMAGE Barrage::imgBlueBig;
IMAGE Barrage::mxtsSpell;
IMAGE Barrage::imgRed;

Barrage::Barrage(float _x, float _y, BulletStyle _style) : Role(_x, _y) {
    style = _style;
    if (imgGreen.getwidth() == 0) loadimage(&imgGreen, L"resource/barrage/mid_bullet_darkGreen.png");
    if (imgBlue.getwidth() == 0) loadimage(&imgBlue, L"resource/barrage/blue_bullet.png");
    if (imgRiceBlue.getwidth() == 0) loadimage(&imgRiceBlue, L"resource/barrage/rice_bullet.png");
    if (imgRiceRed.getwidth() == 0) loadimage(&imgRiceRed, L"resource/barrage/rice_bullet.png");
    if (mxtsSpell.getwidth() == 0) loadimage(&mxtsSpell, L"resource/barrage/etama_big.png");
    if (imgRed.getwidth() == 0) loadimage(&imgRed, L"resource/barrage/red_bullet.png");

    t1 = 0;
    t2 = 0;
    speed = 4.0f;
    rSpeed = 0.0f;
    radian = 0.0f;
    vx = 0.0f;
    vy = 0.0f;
    motion = BulletMotion::LINEAR;
    bornTime = GetTickCount();
    switchDelay = 0;
    motionPhase = 0;
    motionAngle = 0.0f;
    motionSpeed = 0.0f;
    turnRate = 0.0f;
    deceleration = 0.0f;
    targetX = 0.0f;
    targetY = 0.0f;
    originX = _x;
    originY = _y;
    travelled = 0.0f;
    waveAmplitude = 0.0f;
    waveFrequency = 0.0f;
    releaseSpeed = 0.0f;
    radiusGrowth = 0.0f;
    radius = 0.0f;
    alive = false;
    lock = false;
    outBound = false;
    groupID = -1;
    isFriendly = false;
    isRotated = false;
}

Barrage::~Barrage() {
    for (auto* b : barrList) {
        delete b;
    }
    barrList.clear();
}

bool Barrage::appendBullet(Barrage* bullet) {
    if (bullet == nullptr) return false;
    if (barrList.size() >= kMaxLiveBarrages) {
        delete bullet;
        return false;
    }
    barrList.push_back(bullet);
    return true;
}

POINTFLOAT getStarVertex(int index, float r) {
    const float angle = -PI / 2.0f + index * (2.0f * PI / 5.0f);
    return { r * cos(angle), r * sin(angle) };
}

void Barrage::reset() {
    t1 = GetTickCount();
    bornTime = t1;
    motionAngle = 0.0f;
    motionPhase = 0;
}

BulletRect Barrage::getRect(BulletStyle s) {
    switch (s) {
    case BulletStyle::GREEN_: return { 0, 0, darkGreenWidth, darkGreenHeight };
    case BulletStyle::BLUE_: return { 0, 0, darkGreenWidth, darkGreenHeight };
    case BulletStyle::RICE_BULE: return { 100, 0, riceWidth, riceHeight };
    case BulletStyle::RICE_RED: return { 36, 0, riceWidth, riceHeight };
    case BulletStyle::RED_: return { 0, 0, darkGreenWidth, darkGreenHeight };
    case BulletStyle::BLUE_BIG: return { 0, 0, (int)(darkGreenWidth * 1.5f), (int)(darkGreenWidth * 1.5f) };
    default: return { 0, 0, darkGreenWidth, darkGreenHeight };
    }
}

int Barrage::getWidth() const { return getRect(style).w; }
int Barrage::getHeight() const { return getRect(style).h; }

void Barrage::calRotation() {
    const bool isRice = (style == BulletStyle::RICE_BULE || style == BulletStyle::RICE_RED);
    if (!isRice) {
        isRotated = false;
        return;
    }

    BulletRect r = getRect(style);
    IMAGE* imgPtr = (style == BulletStyle::RICE_BULE) ? &imgRiceBlue : &imgRiceRed;
    if (imgPtr->getwidth() == 0) return;

    const float angle = atan2(vy, vx);
    const float theta = PI / 2.0f - angle;
    const int side = (std::max)(r.w, r.h);
    IMAGE squareImg(side, side);
    SetWorkingImage(&squareImg);
    std::memset(GetImageBuffer(&squareImg), 0, side * side * sizeof(DWORD));
    const int offsetX = (side - r.w) / 2;
    const int offsetY = (side - r.h) / 2;
    SetWorkingImage(imgPtr);
    IMAGE tempImg;
    getimage(&tempImg, r.x, r.y, r.w, r.h);
    SetWorkingImage(&squareImg);
    putimage(offsetX, offsetY, &tempImg, SRCPAINT);
    SetWorkingImage(NULL);
    rotateimage(&cacheImg, &squareImg, theta);

    DWORD* pMem = GetImageBuffer(&cacheImg);
    const int pixelCount = cacheImg.getwidth() * cacheImg.getheight();
    for (int i = 0; i < pixelCount; ++i) {
        if ((pMem[i] & 0x00FFFFFF) != 0) pMem[i] |= 0xFF000000;
    }
    isRotated = true;
}

void Barrage::draw() {
    if (isFriendly) {
        const int drawW = 200;
        const int drawH = 200;
        putimagePNG((int)(x - drawW / 2), (int)(y - drawH / 2), 256, 256, &mxtsSpell, 0, 0, drawW, drawH);
        return;
    }
    if (isRotated) {
        const int rw = cacheImg.getwidth();
        const int rh = cacheImg.getheight();
        putimagePNG((int)(x - rw / 2), (int)(y - rh / 2), rw, rh, &cacheImg, 0, 0, rw, rh);
        return;
    }

    IMAGE* imgPtr = nullptr;
    BulletRect r = getRect(style);
    int dstX = r.w;
    int dstY = r.h;
    switch (style) {
    case BulletStyle::GREEN_: imgPtr = &imgGreen; break;
    case BulletStyle::RICE_BULE:
        imgPtr = &imgRiceBlue;
        dstX = (int)(riceWidth * 1.2f); dstY = (int)(riceHeight * 1.2f);
        break;
    case BulletStyle::RICE_RED:
        imgPtr = &imgRiceRed;
        dstX = (int)(riceWidth * 1.2f); dstY = (int)(riceHeight * 1.2f);
        break;
    case BulletStyle::BLUE_BIG:
        imgPtr = &imgBlue;
        r.w = darkGreenWidth; r.h = darkGreenHeight;
        dstX = (int)(darkGreenWidth * 1.5f); dstY = (int)(darkGreenHeight * 1.5f);
        break;
    case BulletStyle::BLUE_: imgPtr = &imgBlue; break;
    case BulletStyle::RED_: imgPtr = &imgRed; break;
    default: break;
    }
    if (imgPtr != nullptr && imgPtr->getwidth() > 0) {
        putimagePNG((int)(x - dstX / 2), (int)(y - dstY / 2), r.w, r.h, imgPtr, r.x, r.y, dstX, dstY);
    }
}

void Barrage::move() {
    if (!alive) return;

    const DWORD now = GetTickCount();
    const DWORD elapsed = (DWORD)(now - bornTime);
    switch (motion) {
    case BulletMotion::LINEAR:
        x += vx;
        y += vy;
        break;

    case BulletMotion::ORBIT:
        // circleMill 的锁定环在固定时间后切线释放，确保不会永久留在 barrList。
        if (switchDelay > 0 && elapsed >= switchDelay) {
            const float tangent = motionAngle + ((turnRate >= 0.0f) ? PI / 2.0f : -PI / 2.0f);
            const float launchSpeed = (releaseSpeed > 0.0f) ? releaseSpeed : 2.5f;
            vx = launchSpeed * cos(tangent);
            vy = launchSpeed * sin(tangent);
            lock = false;
            motion = BulletMotion::LINEAR;
            calRotation();
        }
        else {
            motionAngle += turnRate;
            x = originX + radius * cos(motionAngle);
            y = originY + radius * sin(motionAngle);
        }
        break;

    case BulletMotion::EXPANDING_ORBIT:
        motionAngle += turnRate;
        radius += radiusGrowth;
        x = originX + radius * cos(motionAngle);
        y = originY + radius * sin(motionAngle);
        break;

    case BulletMotion::DELAYED_ACCELERATION:
        if (elapsed >= switchDelay) {
            vx += originX;
            vy += originY;
            x += vx;
            y += vy;
        }
        break;

    case BulletMotion::CURVE_THEN_STRAIGHT:
        if (elapsed < switchDelay) {
            motionAngle += turnRate;
            vx = motionSpeed * cos(motionAngle);
            vy = motionSpeed * sin(motionAngle);
        }
        x += vx;
        y += vy;
        break;

    case BulletMotion::BRAKE_THEN_AIM:
        if (elapsed < switchDelay) {
            if (motionSpeed > 0.0f) {
                motionSpeed = (std::max)(0.0f, motionSpeed - deceleration);
                vx = motionSpeed * cos(motionAngle);
                vy = motionSpeed * sin(motionAngle);
                x += vx;
                y += vy;
            }
        }
        else {
            // targetX/Y 在生成时保存，之后不读取 Hero，因此没有持续自机狙。
            const float dx = targetX - x;
            const float dy = targetY - y;
            const float length = sqrt(dx * dx + dy * dy);
            const float launchSpeed = (releaseSpeed > 0.0f) ? releaseSpeed : motionSpeed;
            if (length > 0.001f) {
                vx = launchSpeed * dx / length;
                vy = launchSpeed * dy / length;
            }
            else {
                vx = 0.0f;
                vy = launchSpeed;
            }
            motion = BulletMotion::LINEAR;
            motionPhase = 1;
            calRotation();
        }
        break;

    case BulletMotion::ORBIT_THEN_RELEASE:
        if (elapsed < switchDelay) {
            motionAngle += turnRate;
            radius += radiusGrowth;
            x = originX + radius * cos(motionAngle);
            y = originY + radius * sin(motionAngle);
        }
        else {
            const float tangent = motionAngle + ((turnRate >= 0.0f) ? PI / 2.0f : -PI / 2.0f);
            vx = releaseSpeed * cos(tangent);
            vy = releaseSpeed * sin(tangent);
            motion = BulletMotion::LINEAR;
            motionPhase = 1;
            calRotation();
        }
        break;

    case BulletMotion::SINE_WAVE:
        // 由基准轨迹重算位置，不在当前 x/y 上累积正弦偏移，避免漂移。
        travelled += motionSpeed + radiusGrowth;
        x = originX + sin(travelled * waveFrequency + motionAngle) * waveAmplitude;
        y = originY + travelled;
        break;
    }
}

void Barrage::collision() {
    const float margin = 80.0f;
    if (x < LeftEdge - margin || x > Right + margin || y < TopEdge - margin || y > Bottom + margin) {
        outBound = true;
        if (!lock) alive = false;
    }
    if (lock && (groupID < 0 || (DWORD)(GetTickCount() - bornTime) >= kForcedLockedLifetime)) {
        lock = false;
        if (outBound) alive = false;
    }
}

void Barrage::Normal(Enemy& e, float bulletSpeed, BulletStyle s) {
    if (!e.isAlive() || !e.fire) return;
    Barrage* bullet = new Barrage(e.x, e.y, s);
    bullet->vy = bulletSpeed;
    bullet->alive = true;
    if (appendBullet(bullet)) Game::Effects.spawn(EffectType::CREATE_BARRAGE, e.x, e.y);
}

void Barrage::straightMill(Enemy& e, float bulletSpeed, float baseAngle, int num, int x0, int y0, int dir, BulletStyle s) {
    if (!e.isAlive() || !e.fire || num <= 0) return;
    for (int i = 0; i < num; ++i) {
        Barrage* bullet = new Barrage((float)x0, (float)y0, s);
        float rad = (baseAngle + i * (360.0f / num)) * PI / 180.0f;
        if (dir < 0) rad = -rad;
        bullet->vx = bulletSpeed * cos(rad);
        bullet->vy = bulletSpeed * sin(rad);
        bullet->alive = true;
        bullet->calRotation();
        if (appendBullet(bullet)) spawnBarrageEffectSampled(i, (float)x0, (float)y0);
    }
    playBarrageSoundThrottled();
}

void Barrage::straightMill2(Enemy& e, float bulletSpeed, float baseAngle, int num, int x0, int y0, int dir, BulletStyle s) {
    straightMill(e, bulletSpeed, baseAngle, num, x0, y0, dir, s);
}

void Barrage::directionalMill(Enemy& e, float bulletSpeed, float baseAngle, int num, int x0, int y0, int dir, BulletStyle s) {
    // 方向已经反映在调用者维护的 baseAngle 中，保持旧 switching 风车的几何方向。
    (void)dir;
    straightMill(e, bulletSpeed, baseAngle, num, x0, y0, 1, s);
}

void Barrage::fireWork(Enemy& e, float bulletSpeed, int num, int x0, int y0, BulletStyle s) {
    straightMill(e, bulletSpeed, 0.0f, num, x0, y0, 1, s);
}

void Barrage::circleMill(Enemy& e, float bulletSpeed, int r, int num, int x0, int y0, BulletStyle s) {
    if (!e.isAlive() || !e.fire || num <= 0 || r <= 0) return;
    const float orbitRadius = (float)r;
    const int currentGroup = ++globalGroupID;
    for (int i = 0; i < num; ++i) {
        Barrage* bullet = new Barrage((float)x0, (float)y0, s);
        const float groupAngle = i * (2.0f * PI / num);
        bullet->motion = BulletMotion::ORBIT;
        bullet->lock = true;
        bullet->groupID = currentGroup;
        bullet->originX = x0 + orbitRadius * cos(groupAngle);
        bullet->originY = y0 + orbitRadius * sin(groupAngle);
        bullet->radius = orbitRadius;
        bullet->motionAngle = atan2((float)y0 - bullet->originY, (float)x0 - bullet->originX);
        bullet->turnRate = bulletSpeed / orbitRadius;
        bullet->switchDelay = 6500;
        bullet->releaseSpeed = (std::max)(2.0f, bulletSpeed);
        bullet->alive = true;
        if (appendBullet(bullet)) spawnBarrageEffectSampled(i, (float)x0, (float)y0);
    }
    playBarrageSoundThrottled();
}

void Barrage::pincerAim(Enemy& e, float targetX, float targetY, float bulletSpeed, int spacing, int pairNum, int x0, int y0, BulletStyle s) {
    if (!e.isAlive() || !e.fire) return;
    for (int i = 1; i <= pairNum; ++i) {
        const int offset = i * spacing;
        Barrage* left = new Barrage((float)(x0 - offset), (float)y0, s);
        const float leftAngle = atan2(targetY - y0, targetX - (x0 - offset));
        left->vx = bulletSpeed * cos(leftAngle);
        left->vy = bulletSpeed * sin(leftAngle);
        left->alive = true;
        left->calRotation();
        if (appendBullet(left)) spawnBarrageEffectSampled((i - 1) * 2, left->x, left->y);

        Barrage* right = new Barrage((float)(x0 + offset), (float)y0, s);
        const float rightAngle = atan2(targetY - y0, targetX - (x0 + offset));
        right->vx = bulletSpeed * cos(rightAngle);
        right->vy = bulletSpeed * sin(rightAngle);
        right->alive = true;
        right->calRotation();
        if (appendBullet(right)) spawnBarrageEffectSampled((i - 1) * 2 + 1, right->x, right->y);
    }
    playBarrageSoundThrottled();
}

void Barrage::randomRain(float bulletSpeed, BulletStyle s) {
    const int halfWidth = getRect(s).w / 2;
    const int minX = LeftEdge + halfWidth;
    const int maxX = Right - halfWidth;
    const int rndX = minX + rand() % (maxX - minX + 1);
    Barrage* bullet = new Barrage((float)rndX, (float)TopEdge, s);
    bullet->vy = bulletSpeed;
    bullet->alive = true;
    if (appendBullet(bullet)) {
        Game::Effects.spawn(EffectType::CREATE_BARRAGE, (float)rndX, (float)TopEdge);
        playBarrageSoundThrottled();
    }
}

void Barrage::fiveStar(Enemy& e, int currentStep, int totalSteps, float orbitRadius, float starRadius, float stretchSpeed,
    float normalAcc, DWORD releaseTime, int starCount, int x0, int y0, BulletStyle s) {
    if (!e.isAlive() || totalSteps <= 0 || starCount <= 0) return;
    if (currentStep == 0) ++globalGroupID;

    const float normalizedProgress = (float)currentStep / (float)totalSteps * 5.0f;
    int edgeIdx = (int)normalizedProgress;
    if (edgeIdx >= 5) edgeIdx = 4;
    const float t = normalizedProgress - edgeIdx;
    const int indices[] = { 4, 1, 3, 0, 2, 4 };
    const POINTFLOAT pStart = getStarVertex(indices[edgeIdx], starRadius);
    const POINTFLOAT pEnd = getStarVertex(indices[edgeIdx + 1], starRadius);
    const float lx = pStart.x + (pEnd.x - pStart.x) * t;
    const float ly = pStart.y + (pEnd.y - pStart.y) * t;
    const float dx = pEnd.x - pStart.x;
    const float dy = pEnd.y - pStart.y;
    const float len = sqrt(dx * dx + dy * dy);
    const float ux = (len > 0.001f) ? dx / len : 0.0f;
    const float uy = (len > 0.001f) ? dy / len : 0.0f;
    const float vxTan = ux * -8.0f * stretchSpeed;
    const float vyTan = uy * -8.0f * stretchSpeed;
    float nx = -uy;
    float ny = ux;
    if (((pStart.x + pEnd.x) * 0.5f) * nx + ((pStart.y + pEnd.y) * 0.5f) * ny > 0.0f) {
        nx = -nx;
        ny = -ny;
    }
    const float accScale = (1.0f / 3.0f - t) * 0.8f;
    const float ax = nx * normalAcc * accScale;
    const float ay = ny * normalAcc * accScale;
    const DWORD now = GetTickCount();

    for (int i = 0; i < starCount; ++i) {
        const float orbitAngle = -PI / 2.0f + i * (2.0f * PI / starCount);
        const float cx = x0 + orbitRadius * cos(orbitAngle);
        const float cy = y0 + orbitRadius * sin(orbitAngle);
        Barrage* bullet = new Barrage(cx + lx, cy + ly, s);
        bullet->motion = BulletMotion::DELAYED_ACCELERATION;
        bullet->vx = vxTan;
        bullet->vy = vyTan;
        bullet->originX = ax;
        bullet->originY = ay;
        bullet->switchDelay = (releaseTime > now) ? (DWORD)(releaseTime - now) : 0;
        bullet->lock = true;
        bullet->groupID = globalGroupID;
        bullet->alive = true;
        if (appendBullet(bullet)) spawnBarrageEffectSampled(i, cx + lx, cy + ly);
    }
    playBarrageSoundThrottled();
}

void Barrage::curvedPetal(Enemy& e, float bulletSpeed, float baseAngle, int num, DWORD curveTime, int dir, float curveTurnRate, int x0, int y0, BulletStyle s) {
    if (!e.isAlive() || !e.fire || num <= 0) return;
    for (int i = 0; i < num; ++i) {
        const float angle = (baseAngle + i * (360.0f / num)) * PI / 180.0f;
        Barrage* bullet = new Barrage((float)x0, (float)y0, s);
        bullet->motion = BulletMotion::CURVE_THEN_STRAIGHT;
        bullet->motionAngle = angle;
        bullet->motionSpeed = bulletSpeed;
        bullet->turnRate = curveTurnRate * ((dir >= 0) ? 1.0f : -1.0f);
        bullet->switchDelay = curveTime;
        bullet->vx = bulletSpeed * cos(angle);
        bullet->vy = bulletSpeed * sin(angle);
        bullet->alive = true;
        bullet->calRotation();
        if (appendBullet(bullet)) spawnBarrageEffectSampled(i, (float)x0, (float)y0);
    }
    playBarrageSoundThrottled();
}

void Barrage::brakeAimRing(Enemy& e, float savedTargetX, float savedTargetY, float bulletSpeed, float baseAngle, int num,
    DWORD waitTime, float brakeRate, int x0, int y0, BulletStyle s) {
    if (!e.isAlive() || !e.fire || num <= 0) return;
    for (int i = 0; i < num; ++i) {
        const float angle = (baseAngle + i * (360.0f / num)) * PI / 180.0f;
        Barrage* bullet = new Barrage((float)x0, (float)y0, s);
        bullet->motion = BulletMotion::BRAKE_THEN_AIM;
        bullet->motionAngle = angle;
        bullet->motionSpeed = bulletSpeed;
        bullet->deceleration = brakeRate;
        bullet->switchDelay = waitTime;
        bullet->targetX = savedTargetX;
        bullet->targetY = savedTargetY;
        bullet->releaseSpeed = bulletSpeed * 1.35f;
        bullet->vx = bulletSpeed * cos(angle);
        bullet->vy = bulletSpeed * sin(angle);
        bullet->alive = true;
        bullet->calRotation();
        if (appendBullet(bullet)) spawnBarrageEffectSampled(i, (float)x0, (float)y0);
    }
    playBarrageSoundThrottled();
}

void Barrage::mirrorSineWall(Enemy& e, float bulletSpeed, float frequency, int perSide, float amplitude, int dir,
    float downwardDrift, int x0, int y0, BulletStyle s) {
    if (!e.isAlive() || !e.fire || perSide <= 0) return;

    const float gameCentre = LeftEdge + WIDTH * 0.5f;
    // dir 明确指定通道向左/右偏移，位置完全由规则计算，不依赖随机数。
    const float safeCentre = gameCentre + ((dir >= 0) ? 24.0f : -24.0f);
    const float safeGap = 42.0f; // 内沿最窄保持 42 像素，通道不依赖随机数。
    const float leftBase = safeCentre - safeGap * 0.5f - amplitude;
    const float rightBase = safeCentre + safeGap * 0.5f + amplitude;
    const float rowSpacing = (HEIGHT + 160.0f) / (float)perSide;
    const float phaseDirection = (dir >= 0) ? 1.0f : -1.0f;

    for (int i = 0; i < perSide; ++i) {
        const float startY = TopEdge - i * rowSpacing;
        const float phase = i * 0.72f * phaseDirection;
        Barrage* left = new Barrage(leftBase + sin(phase) * amplitude, startY, s);
        left->motion = BulletMotion::SINE_WAVE;
        left->originX = leftBase;
        left->originY = startY;
        left->motionAngle = phase;
        left->motionSpeed = bulletSpeed;
        left->radiusGrowth = downwardDrift;
        left->waveAmplitude = amplitude;
        left->waveFrequency = frequency * phaseDirection;
        left->vx = 0.0f;
        left->vy = bulletSpeed + downwardDrift;
        left->alive = true;
        if (appendBullet(left)) spawnBarrageEffectSampled(i * 2, left->x, left->y);

        // 右侧相位取镜像，始终围绕固定中线留下同一条可学习通道。
        Barrage* right = new Barrage(rightBase - sin(phase) * amplitude, startY, s);
        right->motion = BulletMotion::SINE_WAVE;
        right->originX = rightBase;
        right->originY = startY;
        right->motionAngle = PI + phase;
        right->motionSpeed = bulletSpeed;
        right->radiusGrowth = downwardDrift;
        right->waveAmplitude = amplitude;
        right->waveFrequency = frequency * phaseDirection;
        right->vx = 0.0f;
        right->vy = bulletSpeed + downwardDrift;
        right->alive = true;
        if (appendBullet(right)) spawnBarrageEffectSampled(i * 2 + 1, right->x, right->y);
    }
    playBarrageSoundThrottled();
}

void Barrage::orbitRelease(Enemy& e, float bulletSpeed, float angularSpeed, int num, DWORD orbitTime, int dir,
    float growthRate, int x0, int y0, BulletStyle s) {
    if (!e.isAlive() || !e.fire || num <= 0) return;
    const float initialRadius = 42.0f;
    const float direction = (dir >= 0) ? 1.0f : -1.0f;
    for (int i = 0; i < num; ++i) {
        const float angle = i * (2.0f * PI / num);
        Barrage* bullet = new Barrage((float)x0 + initialRadius * cos(angle), (float)y0 + initialRadius * sin(angle), s);
        bullet->motion = BulletMotion::ORBIT_THEN_RELEASE;
        bullet->originX = (float)x0;
        bullet->originY = (float)y0;
        bullet->radius = initialRadius;
        bullet->motionAngle = angle;
        bullet->turnRate = angularSpeed * direction;
        bullet->radiusGrowth = growthRate;
        // 每颗按索引错开 14ms，绝不在同一帧整体释放。
        bullet->switchDelay = orbitTime + i * 14;
        bullet->releaseSpeed = bulletSpeed;
        bullet->alive = true;
        if (appendBullet(bullet)) spawnBarrageEffectSampled(i, bullet->x, bullet->y);
    }
    playBarrageSoundThrottled();
}

void Barrage::update() {
    std::set<int> groupsInside;
    for (auto* bullet : barrList) {
        if (bullet == nullptr) continue;
        bullet->move();
        bullet->collision();
        if (bullet->lock && bullet->groupID >= 0 && !bullet->outBound) groupsInside.insert(bullet->groupID);
    }

    // 同组全部离开后立即解除锁定，并在本帧回收越界对象。
    for (auto* bullet : barrList) {
        if (bullet == nullptr || !bullet->lock || bullet->groupID < 0) continue;
        if (groupsInside.find(bullet->groupID) == groupsInside.end()) {
            bullet->lock = false;
            if (bullet->outBound) bullet->alive = false;
        }
    }

    for (auto it = barrList.begin(); it != barrList.end();) {
        Barrage* bullet = *it;
        if (bullet == nullptr || !bullet->isAlive()) {
            delete bullet;
            it = barrList.erase(it);
        }
        else {
            if (!bullet->outBound) bullet->draw();
            ++it;
        }
    }
}

void Barrage::clearBarrage() {
    for (auto* bullet : barrList) {
        delete bullet;
    }
    barrList.clear();
}
