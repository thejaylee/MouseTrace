#ifndef DRAWER_H
#define DRAWER_H

#include <stdlib.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <windowsx.h>

#define MAX_BARS 100 //number of bars for the histogram

void drawInit(void);
void drawHistogram(HWND, long*, int, int);
//void drawTargetGraph(HWND, float, float);
void drawTracerGraph(HWND, long*, long*, int, int);
void drawHwndToFile(HWND, char*);
void drawCleanup(void);

#endif