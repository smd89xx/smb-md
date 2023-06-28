#include "includes.h"

u32* stopcode_public;
static void joyEvent_BSOD(u16 joy, u16 changed, u16 state)
{
    u16 basetile = TILE_ATTR(PAL3,FALSE,FALSE,FALSE);
    if (changed & state & BUTTON_START)
    {
        switch (*stopcode_public)
        {
        case 0:
        {
            SYS_hardReset();
            break;
        }
        case 1:
        {
            SYS_hardReset();
            break;
        }
        case 2:
        {
            SYS_hardReset();
            break;
        }
        case 3:
        {
            SYS_hardReset();
            break;
        }
        case 4:
        {
            XGM_setPCM(64,bowser_fireball_sfx,sizeof(bowser_fireball_sfx));
            XGM_startPlayPCM(64,15,SOUND_PCM_CH2);
            break;
        }
        default:
        {
            killExec(genericErr);
            break;
        }
        }
    }
}

void killExec(u32 stopcode)
{
    PSG_reset();
    XGM_startPlay(player_down);
    VDP_loadFont(menu_font.tileset,DMA);
    SPR_end();
    VDP_setScreenWidth320();
    VDP_clearPlane(BG_A,TRUE);
    PAL_setColor(0,0x0000);
    VDP_setWindowVPos(FALSE,0);
    VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A,0);
    VDP_setVerticalScroll(BG_A,0);
    u16 basetile = TILE_ATTR(PAL3,FALSE,FALSE,FALSE);
    u16 basetileVRAM = TILE_ATTR_FULL(PAL2,FALSE,FALSE,FALSE,ind);
    u8 x = 18, y = 11;
    VDP_drawImageEx(BG_A,&bsod_frown,basetileVRAM,x,y,FALSE,TRUE);
    char scStr[9] = "00000000";
    intToHex(stopcode,scStr,8);
    VDP_drawTextEx(BG_A,scStr,basetile,x-2,y+4,DMA);
    u32* errMem;
    u32 errVal = 0;
    switch (stopcode)
    {
    case 0:
    {
        errMem = NULL;
        break;
    }
    case 1:
    {
        errMem = &level[0];
        errVal = (level[0] << 8) + level[1];
        break;
    }
    case 2:
    {
        errMem = NULL;
        break;
    }
    case 3:
    {
        errMem = NULL;
        break;
    }
    case 4:
    {
        errMem = 0xA10001;
        errVal = getConsoleRegion();
        break;
    }
    default:
    {
        errMem = NULL;
        break;
    }
    }
    intToHex(errMem,scStr,8);
    VDP_drawTextEx(BG_A,scStr,basetile,x-2,y+5,DMA);
    intToHex(errVal,scStr,8);
    VDP_drawTextEx(BG_A,scStr,basetile,x-2,y+6,DMA);
    stopcode_public = MEM_alloc(sizeof(u32));
    memcpy(stopcode_public,&stopcode,sizeof(u32));
    JOY_setEventHandler(joyEvent_BSOD);
    while (1)
    {
        SYS_doVBlankProcess();
    }
}