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
    playerSwimming
};

typedef struct
{
    u8 x;
    u8 y;
    char label[11];
} Option;

const s16 ind = TILE_USER_INDEX;
bool player = FALSE; // 0 = Mario, 1 = Luigi
u8 playerState = 0;  // 0 = Small, 1 = Big, 2 = Fire
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
Map *lvlBG;
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
u8 playerWidth = 16;
u8 playerHeight = 16;

static void setPalette(u16 *palette)
{
    PAL_setColors(0, palette, 28, DMA);
}

static void spawnPlayer()
{
    u16 basetile = TILE_ATTR(PAL1, FALSE, FALSE, FALSE);
    PAL_setColors(28, playerPalettes[player], 4, DMA);
    player_spr = SPR_addSprite(&mario, fix32ToRoundedInt(player_x), fix32ToRoundedInt(player_y), basetile);
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
        VDP_drawText(" ", 15, 3);
        XGM_setPCM(64, life_sfx, sizeof(life_sfx));
        XGM_startPlayPCM(64, 15, SOUND_PCM_CH1);
    }
    if (score > scoreMax)
    {
        score = scoreMax;
        VDP_drawText("  ", 9, 3);
    }
}

static void timerUpdate()
{
    if (paused == FALSE)
    {
        if (gameTimer > FIX16(0))
        {
            gameTimer -= FIX16(0.025);
        }
        else if (gameTimer <= FIX16(100))
        {
            gameTimer -= FIX16(0.025);
            XGM_setMusicTempo(90);
        }
        if (gameTimer <= FIX16(0))
        {
            gameTimer = FIX16(0);
        }
    }
}

static void qblockGlowCycler()
{
    glowTimer += FIX16(0.09375);
    if (glowTimer >= FIX16(5.75))
    {
        glowTimer = FIX16(0);
    }
    PAL_setColor(8, qblockGlow[fix16ToRoundedInt(glowTimer)]);
}

static void drawLevel()
{
    u16 basetileVRAM = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind);
    VDP_loadTileSet(&smb_tiles, ind, DMA);
    player_x = FIX32(56);
    player_y = FIX32(192);
    switch (level[0])
    {
    case 0:
    {
        switch (level[1])
        {
        case 0:
        {
            setPalette(overworldPalette);
            gameTimer = FIX16(timerSets[0]);
            lvlFG = MAP_create(&lvl11, BG_A, basetileVRAM);
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
    XGM_setPCM(64, pause_sfx, sizeof(pause_sfx));
    XGM_startPlayPCM(64, 15, SOUND_PCM_CH2);
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

static void joyEvent_game(u16 joy, u16 changed, u16 state)
{
    if (changed & state & BUTTON_START)
    {
        paused = !paused;
        pauseChk();
    }
    if (changed & state & BUTTON_A)
    {
        XGM_setPCM(64,coin_sfx,sizeof(coin_sfx));
        XGM_startPlayPCM(64,15,SOUND_PCM_CH1);
        coinCount++;
    }
    else if (changed & state & BUTTON_B)
    {
        XGM_setPCM(64,shell_kick,sizeof(shell_kick));
        XGM_startPlayPCM(64,15,SOUND_PCM_CH1);
        score += 50;
    }
    else if (changed & state & BUTTON_C)
    {
        XGM_setPCM(64,jump_sfx,sizeof(jump_sfx));
        XGM_startPlayPCM(64,15,SOUND_PCM_CH1);
    }
    
    
}

static void gameInit()
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);
    u8 z80Usage;
    drawLevel();
    spawnHUD();
    spawnPlayer();
    SPR_setAnim(player_spr,playerMoving);
    JOY_setEventHandler(joyEvent_game);
    u16 scroll = 0;
    while (1)
    {
        XGM_nextFrame();
        z80Usage = XGM_getCPULoad();
        if (z80Usage >= 100)
        {
            killExec(z80Overload);
        }
        scroll++;
        MAP_scrollTo(lvlFG, scroll, 0);
        updateHUD();
        qblockGlowCycler();
        timerUpdate();
        SPR_update();
        SYS_doVBlankProcess();
    }
}

static void joyEvent_null() {}

static u8 pixelToTile(u8 pixel)
{
    return pixel << 3;
}

static void introScreen()
{
    setPalette(undergroundPalette);
    VDP_clearPlane(BG_A, TRUE);
    spawnHUD();
    SPR_releaseSprite(title_cursor);
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
    u8 introTimer = (u8)161.4f;
    gameTimer = FIX16(timerSets[3] - 1);
    updateHUD();
    JOY_setEventHandler(joyEvent_null);
    while (1)
    {
        introTimer--;
        if (introTimer <= 0)
        {
            SPR_releaseSprite(player_spr);
            gameInit();
        }
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
    VDP_drawImageEx(BG_A, &title_img, basetileVRAM, 0, 4, FALSE, TRUE);
    VDP_drawTextBG(BG_A, "@1985 NINTENDO", 13, 15);
    VDP_drawTextBG(BG_A, "@2023 THEWINDOWSPRO98", 6, 16);
    VDP_drawTextBG(BG_A, "TOP-", titleX, titleY + 5);
    char scoreStr[7] = "000000";
    intToStr(score, scoreStr, 6);
    VDP_drawTextBG(BG_A, scoreStr, titleX + 4, titleY + 5);
    Z80_unloadDriver();
    gameTimer = FIX16(timerSets[0] - 1);
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
    }
}

u8 getConsoleRegion()
{
    u8 consoleType = *(u8 *)0xA10001;
    return consoleType;
}

int main()
{
    SPR_init();
    VDP_setScreenWidth256();
    PAL_setColors(32, bsod_palette, 32, DMA);
    VDP_loadFont(custom_font.tileset, DMA);
    if (getConsoleRegion() == palEUR || getConsoleRegion() == palJPN)
    {
        killExec(badRegion);
    }
    title();
    while (1)
    {
        SYS_doVBlankProcess();
    }
    return (0);
}
