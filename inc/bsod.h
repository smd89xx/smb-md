#pragma once

#include "includes.h"

enum stopCodes {genericErr, lvlOutOfRange, z80Overload, featureNotFound, badRegion, stateOutOfRange, bonusOutOfRange};


/// @brief Shows a BSOD.
/// @param stopcode The error code.
void killExec(u32 stopcode);