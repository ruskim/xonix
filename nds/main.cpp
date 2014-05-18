/* Nintendo ds file */

#include <nds.h>
#include <stdlib.h>
#include <stdio.h>

#include "xonix.h"

typedef struct
{
    int x;
    int y;
}Point;

Point *p = 0;
int p_cnt = 0;
int p_max = 0;

int evils_cnt = 0;
int victory = 0;
int score = 0;
int score_changed = 0;

u8* tileMemory;
u16* mapMemory;

#define W 32
#define H 24
#define BS 8

u8 base_tile[64] = 
{
	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,1,2,2,2,2,1,0,
	0,1,2,2,2,2,1,0,
	0,1,2,2,2,2,1,0,
	0,1,2,2,2,2,1,0,
	0,1,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0
};

enum {
    tFree=0,
    tBusy,
    tEvil,
    tPlayer,
    tPath,
    tMax
};

u8 free_tile[64];
u8 busy_tile[64];
u8 evil_tile[64];
u8 player_tile[64];
u8 path_tile[64];

u8 *tiles[tMax] = {
    free_tile,
    busy_tile,
    evil_tile,
    player_tile,
    path_tile
};

void set_tile(int x, int y, int t)
{
    mapMemory[x+y*32] = t;
}

void gen_tile(u8 *tile, u8 c)
{
    for(int i=0; i<64; i++) {
        if( base_tile[i] == 2) {
            tile[i] = c;
        } else {
            tile[i] = base_tile[i];
        }
    }
}


void mk_tiles()
{
    BG_PALETTE[tFree]   = RGB15(31,31,31);
	BG_PALETTE[tBusy]   = RGB15(21,21,21);
	BG_PALETTE[tEvil]   = RGB15(0,0,0);
	BG_PALETTE[tPlayer] = RGB15(0,0,21);
	BG_PALETTE[tPath]   = RGB15(0,30,0);

    for( int i=0; i<tMax; i++) {
        gen_tile( tiles[i], i);
    }

    for( int i=0; i<tMax; i++) {
        swiCopy( tiles[i], tileMemory+64*i, 32);
    }
}

void draw_block( int x, int y, int busy)
{
    if( busy) {
        set_tile(x, y, tBusy);
    } else {
        set_tile(x, y, tFree);
    }
}


void clear_screen(void)
{
    for( int x=0; x<W; x++) {
        for( int y=0; y<H; y++) {
            draw_block(x, y, 0);
        }
    }
}

void add_changes(int x, int y)
{
    Point *pt;
    if(p_cnt == p_max) {
        p_max += 10;
        p = (Point *)realloc( (void *)p, p_max*sizeof(Point));
    }
    pt = p + p_cnt;
    pt->x = x;
    pt->y = y;
    p_cnt++;
}

void xonix_callback(void *tag, XonixEvent *e)
{
    switch(e->et) {
        case eObjAdded:
            if( e->ot == objBlock) {
                draw_block(e->x, e->y, 1);
            } else {
                add_changes(e->x, e->y);
            }
            break;

        case eObjMoved:
            add_changes(e->x, e->y);
            add_changes(e->prev_x, e->prev_y);
            break;

        case eObjDeleted:
            add_changes(e->x, e->y);
            break;

        case eScore:
            score = 100 - e->x;
            score_changed = 1;
            break;

        case eVictory:
            victory = 1;
            break;

        default:
            break;
    }
}

void draw_point(Point *pt)
{
    Cell c;
    int x = pt->x;
    int y = pt->y;

    xonix_get_cell(x, y, &c);

    if( xonix_cell_is(&c, EVIL)) {
        set_tile(x, y, tEvil);
    } else if( xonix_cell_is(&c, PLAYER)) {
        set_tile(x, y, tPlayer);
    } else if( xonix_cell_is(&c, PATH)) {
        set_tile(x, y, tPath);
    } else if( xonix_cell_is(&c, BLOCK)) {
        set_tile(x, y, tBusy);
    } else {
        set_tile(x, y, tFree);
    }
}

void process_changes()
{
    for( int i=0; i<p_cnt; i++) {
        draw_point(p+i);
    }
}


void init_level()
{
    clear_screen();
    consoleClear();
    victory = 0;
    xonix_free();
    evils_cnt++;
    p_cnt = 0;
    xonix_init(32, 24, 17, evils_cnt, xonix_callback, 0);
    process_changes();
    score_changed = 1;
    score = 0;
}

int main(void) 
{
    touchPosition touch;

    PrintConsole bottomScreen;

    videoSetMode(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);

    tileMemory = (u8*)BG_TILE_RAM(1);
	mapMemory = (u16*)BG_MAP_RAM(0);

    REG_BG0CNT = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1);

    mk_tiles();

    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    consoleInit(&bottomScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

    consoleSelect(&bottomScreen);

    init_level();

    while(1)
    {
        int key_mask;

        swiWaitForVBlank();
        scanKeys();
        key_mask = 0;

        if( keysHeld() & KEY_UP ) {
            key_mask |= X_KEY_UP;
        }

        if( keysHeld() & KEY_DOWN ) {
            key_mask |= X_KEY_DOWN;
        }

        if( keysHeld() & KEY_LEFT ) {
            key_mask |= X_KEY_LEFT;
        }

        if( keysHeld() & KEY_RIGHT ) {
            key_mask |= X_KEY_RIGHT;
        }

        p_cnt = 0;
        xonix_advance(key_mask);
        process_changes();

        if( score_changed) {
            iprintf("\x1b[10;10HCaptured %i%%", score);
            score_changed = 0;
        }

        if( victory) {
            iprintf("\x1b[2;9HLevel cleared!\n");
            iprintf("\x1b[4;1HPress 'A' or touch to advance\n");
            touchRead(&touch);
            if( (touch.px != 0) || (touch.py != 0) || (keysHeld() & KEY_A )) {
                init_level();
            }
        }
    }

    return 0;
}

