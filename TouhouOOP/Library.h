#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <windows.h>
#include <graphics.h> 
#include <conio.h>
#include <vector>
#include <chrono>
#include <set>
#include <algorithm>
#define NOMINMAX
#pragma comment(lib, "Msimg32.lib")
#pragma warning(disable: 28159)
const int WIDTH = 750;	
const int HEIGHT = 900;
const int screenWidth = 1280;
const int screenHeight = 960;
const int LeftEdge = 32;
const int TopEdge = 30;
const int Right = LeftEdge + WIDTH;
const int Bottom = TopEdge + HEIGHT;
const int CentralX = 375;
const int CentralY = 250;
const float PI = 3.1415926f;