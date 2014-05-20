/* Nintendo ds file */

#include <nds.h>
#include <stdlib.h>
#include <stdio.h>


#include "xonix.h"

//int ticks=0;

typedef struct
{
    int x;
    int y;
}Point;

Point *p = 0;
int p_cnt = 0;
int p_max = 0;


typedef struct
{
    int x;
    int y;
    int t_len;
    int t_done;
    int prev_x;
    int prev_y;
    u16 *gfx;
    int oam_idx;
    int visible;
} Sprite;

Sprite *sprites = 0;
int s_max = 21;
int max_s_id = -1;

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

u8 sprite[64] = 
{
	0,0,0,0,0,0,0,0,
	0,2,2,2,2,2,2,0,
	0,2,0,0,0,0,2,0,
	0,2,0,0,0,0,2,0,
	0,2,0,0,0,0,2,0,
	0,2,0,0,0,0,2,0,
	0,2,2,2,2,2,2,0,
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

u16 *gfx_evil = 0;
u16 *gfx_player = 0;

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
    SPRITE_PALETTE[tFree]   = BG_PALETTE[tFree]   = RGB15(31,31,31);
	SPRITE_PALETTE[tBusy]   = BG_PALETTE[tBusy]   = RGB15(21,21,21);
	SPRITE_PALETTE[tEvil]   = BG_PALETTE[tEvil]   = RGB15(0,0,0);
	SPRITE_PALETTE[tPlayer] = BG_PALETTE[tPlayer] = RGB15(0,0,21);
	SPRITE_PALETTE[tPath]   = BG_PALETTE[tPath]   = RGB15(0,30,0);

    for( int i=0; i<tMax; i++) {
        gen_tile( tiles[i], i);
    }

    for( int i=0; i<tMax; i++) {
        swiCopy( tiles[i], tileMemory+64*i, 32);
    }
}

void mk_sprites()
{
    gfx_evil   = oamAllocateGfx(&oamMain, SpriteSize_8x8,SpriteColorFormat_256Color);
    gfx_player = oamAllocateGfx(&oamMain, SpriteSize_8x8,SpriteColorFormat_256Color);

	for(int i = 0; i < 8 * 8 / 2; i++)
	{
		gfx_evil[i]   = ((u16 *)evil_tile)[i];
		gfx_player[i] = ((u16 *)player_tile)[i];
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

int sprite_id( XonixEvent *e)
{
    if( e->ot == objEvil) {
        return e->id - EVIL_ID_SHIFT + 1;
    } 
    return 0;
}

void add_sprite( XonixEvent *e)
{
    int id = sprite_id(e);
    if( id >= s_max) {
        sprites = (Sprite *)realloc(sprites, (id+1)*sizeof(Sprite));
        s_max = id + 1;
    }

    Sprite *s = sprites+id;
    s->x = e->x*BS;
    s->y = e->y*BS;
    s->oam_idx = id;
    if( e->ot == objPlayer) {
        s->gfx = gfx_player;
    } else {
        s->gfx = gfx_evil;
    }
    s->t_len = 0;
    s->t_done = 0;
    s->visible = 1;

    oamSet(&oamMain, //main graphics engine context
        s->oam_idx,           //oam index (0 to 127)  
        s->x,
        s->y,
        0,                    //priority, lower renders last (on top)
        0,					  //this is the palette index if multiple palettes or the alpha value if bmp sprite	
        SpriteSize_8x8,     
        SpriteColorFormat_256Color, 
        s->gfx,                  //pointer to the loaded graphics
        -1,                  //sprite rotation data  
        false,               //double the size when rotating?
        false,			//hide the sprite?
        false, false, //vflip, hflip
        false	//apply mosaic
        );
}

void move_sprite( XonixEvent *e)
{
    int id = sprite_id(e);
    Sprite *s = sprites + id;

    s->x = e->x*BS;
    s->y = e->y*BS;

    s->prev_x = e->prev_x*BS;
    s->prev_y = e->prev_y*BS;

    s->t_len = e->t_len;
    s->t_done = 0;

    //oamSetXY(&oamMain, s->oam_idx, s->x, s->y);
}

void delete_sprite( XonixEvent *e)
{
    int id = sprite_id(e);
    Sprite *s = sprites + id;

    oamSetHidden(&oamMain, s->oam_idx, true);
    s->visible = 0;
}

void tick_sprite( Sprite *s)
{
    if( s->visible == 0) {
        return;
    }
    //oamSetAffineIndex(&oamMain, s->oam_idx, 0, 0);

    if( s->t_len == 0) {
        return;
    }
    s->t_len--;
    s->t_done++;

    int x, y;
    x = (s->x*s->t_done + s->prev_x*s->t_len)/(s->t_done + s->t_len);
    y = (s->y*s->t_done + s->prev_y*s->t_len)/(s->t_done + s->t_len);
    oamSetXY(&oamMain, s->oam_idx, x, y);
    if( s->t_len == 0) {
        add_changes(s->prev_x/BS,s->prev_y/BS);
    }

}

void xonix_callback(void *tag, XonixEvent *e)
{
    switch(e->et) {
        case eObjAdded:
            if( e->ot == objBlock) {
                draw_block(e->x, e->y, 1);
            } else if (e->ot == objPath) {
                //add_changes(e->x, e->y);
            } else {
                add_sprite(e);
            }
            break;

        case eObjMoved:
            move_sprite(e);
            //add_changes(e->x, e->y);
            add_changes(e->prev_x, e->prev_y);
            break;

        case eObjDeleted:
            if( (e->ot == objBlock) || (e->ot == objPath)) {
                add_changes(e->x, e->y);
            } else {
                delete_sprite(e);
            }
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

    if( xonix_cell_is(&c, PATH)) {
        set_tile(x, y, tPath);
    } else if( xonix_cell_is(&c, BLOCK)) {
        draw_block(x, y, 1);
    } else {
        draw_block(x, y, 0);
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
    for( int i=0; i<s_max; i++) {
        Sprite *s = sprites+i;
        if( s->visible != 0) {
            oamSetHidden(&oamMain, s->oam_idx, true);
            s->visible = 0;
        }
    }
    clear_screen();
    consoleClear();
    victory = 0;
    xonix_free();
    evils_cnt++;
    p_cnt = 0;
//    ticks = 0;
    xonix_init(32, 24, 17, evils_cnt, xonix_callback, 0);
    process_changes();
    score_changed = 1;
    score = 0;
}

int main(void) 
{
    PrintConsole bottomScreen;
//    int angle;

    int ss = sizeof(Sprite)*s_max;
    sprites = (Sprite *)malloc( ss);
    memset( sprites, 0, ss);

    vramSetBankA(VRAM_A_MAIN_SPRITE);
	vramSetBankB(VRAM_B_MAIN_BG_0x06000000);
    vramSetBankC(VRAM_C_SUB_BG);

    videoSetMode(MODE_0_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D_LAYOUT | DISPLAY_BG0_ACTIVE);
    videoSetModeSub(MODE_0_2D);

    /* background init */
    tileMemory = (u8*)BG_TILE_RAM(1);
	mapMemory = (u16*)BG_MAP_RAM(0);
    REG_BG0CNT = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_PRIORITY_3;
    mk_tiles();

    /* sprites init */
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    mk_sprites();

    /* console init */
    consoleInit(&bottomScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    consoleSelect(&bottomScreen);

    init_level();

    while(1)
    {
        int key_mask;

        swiWaitForVBlank();

        //ticks++;

        //angle = degreesToAngle(3*ticks);
        //oamRotateScale(&oamMain, 0, angle, intToFixed(1, 8), intToFixed(1, 8));

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
        for( int i=0; i<s_max; i++) {
            tick_sprite( sprites+i);
        }
        process_changes();

        if( score_changed) {
            iprintf("\x1b[10;10HCaptured %i%%", score);
            score_changed = 0;
        }

        if( victory) {
            iprintf("\x1b[2;9HLevel cleared!\n");
            iprintf("\x1b[4;1HPress 'A' or touch to advance\n");
            if( (keysHeld() & KEY_TOUCH) || (keysHeld() & KEY_A )) {
                init_level();
            }
        }
        oamUpdate(&oamMain);
    }

    return 0;
}

