#pragma once

#include "includes.h"

extern u8 level[2];
extern const s16 ind;
extern u8 playerState;
extern u8 bonusScreen;

u8 getConsoleRegion();
static void introScreen();
static void joyEvent_null();
static void death();
static void gameInit(bool initType);