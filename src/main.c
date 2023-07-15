#include "includes.h"

enum maxVals
{
    scoreMax = 999950,
    coinMax = 99
};
enum consoleRegions
{
    ntscUSA = 0xA0,
    ntscJPN = 0x20,
    palEUR = 0xE0,
    palJPN = 0x60
};
enum optionPos
{
    titleX = 11,
    titleY = 19
};
enum screenSize
{
    horzRes = 256,
    vertRes = 224
};
enum playerAnimations
{
    playerIdle,
    playerMoving,
    playerSkidding,
    playerJumping,
    playerDead,
    playerClimbing,
    playerSwimming,
    playerFiring,
    playerCrouching = playerDead
};
enum levelTypes
{
    lvlTypeOverworld,
    lvlTypeUnderground,
    lvlTypeCastle,
    lvlTypeWater,
    lvlTypeNight,
    lvlTypeOverworldMushrooms,
};

enum camScrlBounds
{
    leftCamBnd = 112,
    rightCamBnd = 112
};

typedef struct
{
    u8 x;
    u8 y;
    char label[11];
} Option;

const s16 ind = TILE_USER_INDEX;
bool player = FALSE; // 0 = Mario, 1 = Luigi
u8 playerState = 0;  // 0 = Small, 1 = Big, 2 = Fire, 3 = Growing/Shrinking
const char playerNames[2][6] = {"MARIO", "LUIGI"};
fix16 glowTimer = FIX16(0);
fix16 gameTimer;
u32 score = 0;
u8 lives = 3;
u8 coinCount = 0;
u8 level[2] = {0, 0};
const u16 timerSets[4] = {401, 301, 201, 001};
bool paused = FALSE;
const Option titleOptions[2] =
    {
        {titleX, titleY, "MARIO GAME"},
        {titleX, titleY + 2, "LUIGI GAME"}};
Map *lvlFG;
Sprite *title_cursor;
Sprite *player_spr;
fix32 player_x;
fix32 player_y;
fix32 player_spd_x;
fix32 player_spd_y;
const u16 mapWidths[8][4] =
    {
        {3376, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
};
const u8 playerWidth = 16;
u8 playerHeight = 16;
s16 new_cam_x;
s16 cam_y;
bool isSprinting = FALSE;
bool isJumping = FALSE;
u8 levelType = 0; // See levelTypes
const u8 playerYStartPositions[4] = {48,64,192,96};
bool canScroll = TRUE;
u8 bonusScreen = 255;
u16 mapWidth;

static void camPos()
{
    if (canScroll == FALSE)
    {
        return;
    }
    s16 px = fix32ToRoundedInt(player_x);
    s16 cam_x;
    s16 scrn_x = px - cam_x;
    if (scrn_x > rightCamBnd)
    {
        new_cam_x = px - rightCamBnd;
    }
    else if (scrn_x < leftCamBnd)
    {
        new_cam_x = new_cam_x;
    }
    else
    {
        new_cam_x = cam_x;
    }
    if (new_cam_x < 0)
    {
        new_cam_x = 0;
    }
    else if (new_cam_x > (mapWidths[level[0]][level[1]] - horzRes))
    {
        new_cam_x = mapWidths[level[0]][level[1]] - horzRes;
    }
    if ((cam_x != new_cam_x))
    {
        cam_x = new_cam_x;
    }
    MAP_scrollTo(lvlFG,cam_x,cam_y);
    VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_B,-cam_x >> 1);
}

static void setPalette(u16 *palette)
{
    PAL_setColors(0, palette, 28, DMA);
}

static void spawnPlayer()
{
    u16 basetile = TILE_ATTR(PAL1, FALSE, FALSE, FALSE);
    u16 px = fix32ToInt(player_x) - new_cam_x;
    u16 py = fix32ToInt(player_y);
    switch (playerState)
    {
    case 0:
    {
        PAL_setColors(28, playerPalettes[player], 4, DMA);
        player_spr = SPR_addSprite(&mario, px, py, basetile);
        break;
    }
    case 1:
    {
        PAL_setColors(28, playerPalettes[player], 4, DMA);
        player_spr = SPR_addSprite(&big_mario, px, py, basetile);
        break;
    }
    case 2:
    {
        PAL_setColors(28, playerPalettes[player + 2], 4, DMA);
        player_spr = SPR_addSprite(&big_mario, px, py, basetile);
        break;
    }
    case 3:
    {
        player_y -= FIX32(16);
        PAL_setColors(28, playerPalettes[player], 4, DMA);
        player_spr = SPR_addSprite(&mario_scale, px, py, basetile);
        break;
    }
    default:
    {
        killExec(stateOutOfRange);
        break;
    }
    }
}

static void spawnHUD()
{
    char levelStr[2][2] = {"1", "1"};
    VDP_setWindowVPos(FALSE, 4);
    VDP_setTextPlane(WINDOW);
    VDP_drawText(playerNames[player], 3, 2);
    VDP_drawText("ab", 11, 3);
    VDP_drawText("WORLD", 18, 2);
    intToStr(level[0] + 1, levelStr[0], 1);
    VDP_drawText(levelStr[0], 19, 3);
    intToStr(level[1] + 1, levelStr[1], 1);
    VDP_drawText(levelStr[1], 21, 3);
    VDP_drawText("-", 20, 3);
    VDP_drawText("TIME", 25, 2);
}

static void updateHUD()
{
    char timerStr[4] = "000";
    char scoreStr[7] = "000000";
    char coinStr[3] = "00";
    s16 timerInt = fix16ToInt(gameTimer);
    intToStr(timerInt, timerStr, 3);
    VDP_drawText(timerStr, 26, 3);
    intToStr(coinCount, coinStr, 2);
    VDP_drawText(coinStr, 13, 3);
    intToStr(score, scoreStr, 6);
    VDP_drawText(scoreStr, 3, 3);
    if (coinCount > coinMax)
    {
        coinCount = 0;
        lives++;
        MDS_request(MDS_SE1,BGM_SMB11UP);
        VDP_drawText(" ", 15, 3);
    }
    if (score > scoreMax)
    {
        score = scoreMax;
        VDP_drawText("  ", 9, 3);
    }
}

static void qblockGlowCycler()
{
    glowTimer += FIX16(0.1);
    if (glowTimer >= FIX16(5.7))
    {
        glowTimer = FIX16(0);
    }
    PAL_setColor(8, qblockGlow[levelType][fix16ToRoundedInt(glowTimer)]);
}

static void playerMove()
{
    s16 px = fix32ToRoundedInt(player_x);
    s16 py = fix32ToRoundedInt(player_y);
    if (canScroll == TRUE)
    {
        mapWidth = mapWidths[level[0]][level[1]];
    }
    player_x += player_spd_x;
    player_y += player_spd_y;
    if (isSprinting == TRUE)
    {
        player_spd_x = fix32Mul(player_spd_x,FIX32(1.05));
        if (player_spd_x >= FIX32(2.25))
        {
            player_spd_x = FIX32(2.25);
        }
        else if (player_spd_x <= FIX32(-2.25))
        {
            player_spd_x = FIX32(-2.25);
        }
    }
    if (player_x < FIX32(new_cam_x))
    {
        player_x = FIX32(new_cam_x);
    }
    else if (player_x > FIX32(mapWidth - playerWidth))
    {
        player_x = FIX32(mapWidth - playerWidth);
        MDS_request(MDS_BGM,BGM_S3CLEAR);
    }
    if (player_y <= FIX32(0))
    {
        player_y = FIX32(0);
        MDS_request(MDS_SE1,BGM_SMB1BUMP);
    }
    SPR_setPosition(player_spr, px - new_cam_x, py);
}

static void death()
{
    SPR_releaseSprite(player_spr);
    playerState = 0;
    spawnPlayer();
    SPR_setAnim(player_spr, playerDead);
    MDS_request(MDS_BGM,BGM_SMB1MISS);
    JOY_setEventHandler(joyEvent_null);
    u8 deathTimer = 240;
    while (1)
    {
        SYS_doVBlankProcess();
        SPR_update();
        qblockGlowCycler();
        MDS_update();
        player_y += FIX32(0.5);
        playerMove();
        if (player_y == FIX32(vertRes-1))
        {
            player_y = FIX32(vertRes + 1);
        }
        deathTimer--;
        if (deathTimer == 0)
        {
            lives--;
            introScreen();
        }
    }
}

static void timerUpdate()
{
    if (gameTimer > FIX16(0))
    {
        gameTimer -= FIX16(0.025);
    }
    else if (gameTimer <= FIX16(100))
    {
        gameTimer -= FIX16(0.025);
    }
    if (gameTimer <= FIX16(0))
    {
        gameTimer = FIX16(0);
        death();
    }
}

static void setLevelType(u8 lvlType)
{
    levelType = lvlType;
    switch (levelType)
    {
    case 0:
    {
        setPalette(overworldPalette);
        MDS_request(MDS_BGM,BGM_CLI2);
        break;
    }
    case 1:
    {
        setPalette(undergroundPalette);
        MDS_request(MDS_BGM,BGM_CLI2);
        break;
    }
    default:
    {
        break;
    }
    }
}

static void drawLevel()
{
    u16 basetileVRAM = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind + title_img.tileset->numTile);
    player_x = FIX32(42);
    switch (level[0])
    {
    case 0:
    {
        switch (level[1])
        {
        case 0:
        {
            setLevelType(lvlTypeOverworld);
            player_y = FIX32(playerYStartPositions[2]);
            gameTimer = FIX16(timerSets[0]);
            lvlFG = MAP_create(&lvl11, BG_A, basetileVRAM);
            VDP_setTileMapEx(BG_B,&smb1_bg_hills,basetileVRAM,0,0,0,0,64,28,DMA);
            MAP_scrollTo(lvlFG, 0, 0);
            MEM_free(lvlFG);
            break;
        }
        default:
        {
            break;
        }
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

static void pauseChk()
{
    MDS_pause(MDS_BGM,paused);
    MDS_request(MDS_SE1,BGM_SMB1PAUSE);
    switch (paused)
    {
    case TRUE:
    {
        break;
    }
    case FALSE:
    {
        break;
    }
    default:
    {
        break;
    }
    }
}

static void gameoverScreen()
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A,0);
    setPalette(undergroundPalette);
    MDS_request(MDS_BGM,BGM_SMB1GAMEOVER);
    VDP_drawTextBG(BG_A, "GAME OVER", 11, 15);
    u16 gameoverTimer = 300;
    while (1)
    {
        gameoverTimer--;
        if (gameoverTimer == 0)
        {
            SYS_hardReset();
        }
        SYS_doVBlankProcess();
        MDS_update();
        qblockGlowCycler();
    }
}

static void playerJump()
{
    
}

static u8 pixelToTile(u8 pixel)
{
    return pixel << 3;
}

static void joyEvent_game(u16 joy, u16 changed, u16 state)
{
    if (joy != JOY_1)
    {
        return;
    }
    if (changed & state & BUTTON_START)
    {
        paused = !paused;
        pauseChk();
    }
    if (changed & state & BUTTON_A)
    {
        coinCount++;
        MDS_request(MDS_SE1,BGM_SMB1COIN);
    }
    else if (changed & state & BUTTON_B)
    {
        score += 50;
        MDS_request(MDS_SE1,BGM_SMB1KICK);
    }
    else if (changed & state & BUTTON_C)
    {
        isJumping = TRUE;
        MDS_request(MDS_SE1,BGM_SMWJUMP);
        playerJump();
    }
    else if (changed & state & BUTTON_X)
    {
        bonusScreen++;
        gameInit(TRUE);
    }
    if (state & BUTTON_LEFT)
    {
        SPR_setAnim(player_spr,playerMoving);
        SPR_setHFlip(player_spr,TRUE);
        player_spd_x = FIX32(-2.25);
    }
    else if (state & BUTTON_RIGHT)
    {
        SPR_setAnim(player_spr,playerMoving);
        SPR_setHFlip(player_spr,FALSE);
        player_spd_x = FIX32(2.25);
    }
    else
    {
        SPR_setAnim(player_spr,playerIdle);
        player_spd_x = FIX32(0);
    }
    if (state & BUTTON_UP)
    {
        player_spd_y = FIX32(-2.25);
    }
    else if (state & BUTTON_DOWN)
    {
        player_spd_y = FIX32(2.25);
    }
    else
    {
        player_spd_y = FIX32(0);
    }
}

static void drawBonus()
{
    u16 basetileVRAM = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind + title_img.tileset->numTile);
    player_x = FIX32(pixelToTile(3) + (bonusScreen << 8));
    player_y = FIX32(playerYStartPositions[0]);
    if (bonusScreen > 4)
    {
        killExec(bonusOutOfRange);
    }
    mapWidth = horzRes + (horzRes * bonusScreen);
    canScroll = FALSE;
    new_cam_x = bonusScreen << 8;
    setLevelType(lvlTypeUnderground);
    lvlFG = MAP_create(&smb1_bonuses,BG_A,basetileVRAM);
    MAP_scrollTo(lvlFG,new_cam_x,0);
    MEM_free(lvlFG);
    MDS_request(MDS_SE1,BGM_SMB1PIPE);
}

static void gameInit(bool initType)
{
    player_spd_x = 0;
    player_spd_y = 0;
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    SPR_reset();
    spawnHUD();
    if (initType == FALSE)
    {
        drawLevel();
    }
    else
    {
        drawBonus();
    }
    spawnPlayer();
    SPR_setAnim(player_spr, playerIdle);
    JOY_setEventHandler(joyEvent_game);
    while (1)
    {
        MDS_update();
        updateHUD();
        SYS_doVBlankProcess();
        if (paused == FALSE)
        {
            camPos();
            timerUpdate();
            qblockGlowCycler();
            SPR_update();
            playerMove();
            playerJump();
        }
    }
}

static void joyEvent_null() {}

static void introScreen()
{
    if (lives == 0)
    {
        gameoverScreen();
    }
    SPR_reset();
    new_cam_x = 0;
    levelType = lvlTypeUnderground;
    VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A,new_cam_x);
    setPalette(undergroundPalette);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B,TRUE);
    spawnHUD();
    spawnPlayer();
    SPR_setPosition(player_spr, pixelToTile(12), pixelToTile(13));
    VDP_drawTextBG(BG_A, "b", 15, 14);
    char livesStr[3];
    intToStr(lives, livesStr, 2);
    VDP_drawTextBG(BG_A, livesStr, 17, 14);
    char levelStr[4];
    intToStr(level[0] + 1, levelStr, 1);
    VDP_drawTextBG(BG_A, levelStr, 17, 10);
    VDP_drawTextBG(BG_A, "-", 18, 10);
    intToStr(level[1] + 1, levelStr, 1);
    VDP_drawTextBG(BG_A, levelStr, 19, 10);
    VDP_drawTextBG(BG_A, "WORLD", 11, 10);
    u8 introTimer = 161;
    updateHUD();
    JOY_setEventHandler(joyEvent_null);
    while (1)
    {
        introTimer--;
        if (introTimer <= 0)
        {
            SPR_releaseSprite(player_spr);
            gameInit(FALSE);
        }
        MDS_update();
        SPR_update();
        qblockGlowCycler();
        SYS_doVBlankProcess();
    }
}

static void updateCursor()
{
    SPR_setPosition(title_cursor, pixelToTile(titleOptions[player].x - 2), pixelToTile(titleOptions[player].y));
}

static void joyEvent_title(u16 joy, u16 changed, u16 state)
{
    if (changed & state & BUTTON_DIR)
    {
        player = !player;
        updateCursor();
    }
    if (changed & state & BUTTON_START)
    {
        introScreen();
    }
}

static void title()
{
    setPalette(overworldPalette);
    u16 basetileVRAM = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind);
    u16 basetile = TILE_ATTR(PAL1, FALSE, FALSE, FALSE);
    VDP_setTileMapEx(BG_A,title_img.tilemap,basetileVRAM,5,4,0,0,22,11,DMA);
    VDP_setTileMapEx(BG_A, &title_map, basetileVRAM + title_img.tileset->numTile, 0, 18, 0, 0, 32, 10, DMA);
    spawnPlayer();
    SPR_setPosition(player_spr, 42, pixelToTile(24));
    VDP_drawTextBG(BG_A, "@1985 NINTENDO", 13, 15);
    VDP_drawTextBG(BG_A, "@2023 THEWINDOWSPRO98", 6, 16);
    VDP_drawTextBG(BG_A, "TOP-", titleX, titleY + 5);
    char scoreStr[7] = "000000";
    intToStr(score, scoreStr, 6);
    VDP_drawTextBG(BG_A, scoreStr, titleX + 4, titleY + 5);
    spawnHUD();
    updateHUD();
    VDP_drawText("   ", 26, 3);
    for (u8 i = 0; i < 2; i++)
    {
        Option o = titleOptions[i];
        VDP_drawTextBG(BG_A, o.label, o.x, o.y);
    }
    title_cursor = SPR_addSprite(&cursor, 0, 0, basetile);
    updateCursor();
    JOY_setEventHandler(&joyEvent_title);
    while (1)
    {
        SPR_update();
        SYS_doVBlankProcess();
        qblockGlowCycler();
        MDS_update();
    }
}

u8 getConsoleRegion()
{
    u8 consoleType = *(u8 *)0xA10001;
    return consoleType;
}

int main(bool resetType)
{
    if (resetType == 0)
    {
        SYS_hardReset();
    }
    SPR_init();
    VDP_setScreenWidth256();
    Z80_unloadDriver();
    MDS_init(mdsseqdat,mdspcmdat);
    PAL_setColors(32, bsod_palette, 32, DMA);
    VDP_loadFont(custom_font.tileset, DMA);
    VDP_loadTileSet(title_img.tileset,ind,DMA);
    VDP_loadTileSet(&smb_tiles,ind+title_img.tileset->numTile,DMA);
    u8 consoleRegion = getConsoleRegion();
    if (consoleRegion == palEUR || consoleRegion == palJPN)
    {
        killExec(badRegion);
    }
    title();
    while (1)
    {
        SYS_doVBlankProcess();
        MDS_update();
    }
    return (0);
}
