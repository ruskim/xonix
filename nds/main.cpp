/* Nintendo ds file */

#include <nds.h>
#include <stdlib.h>
#include "xonix.h"
 
typedef struct
{
    int x;
    int y;
}Point;

Point *p = 0;
int p_cnt = 0;
int p_max = 0;


#define P(x,y) VRAM_A[(x) + (y) * SCREEN_WIDTH]
#define W 32
#define H 24
#define BS 8

void draw_block( int x, int y, int busy)
{
    x = x*BS;
    y = y*BS;

    for(int i=0;i<BS;i++) {
        for(int j = 0; j<BS; j++) {
            if( i < BS-1 && i > 0 && j < BS-1 && j > 0) {
                if( busy) {
                    P(x+i,y+j) = RGB15(21,21,21);
                } else {
                    if( (i == BS-2) || (i == 1) || (j == BS-2) || (j == 1)) {
                        P(x+i,y+j) = RGB15(21,21,21);
                    } else {
                        P(x+i,y+j) = RGB15(31,31,31);
                    }
                }
            } else {
                P(x+i,y+j) = RGB15(31,31,31);
            }
        }
    }
}

void draw_creature( int x, int y, int color)
{
    x = x*BS;
    y = y*BS;
    
    for( int i=2; i<6; i++) {
        for( int j=2; j<6; j++) {
            P(x+i, y+j) = color;
        }
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

    draw_block(x, y, xonix_cell_is(&c, BLOCK));

    if( xonix_cell_is(&c, EVIL)) {
        draw_creature(x, y, RGB15(0,0,0));
    }
    if( xonix_cell_is(&c, PATH)) {
        draw_creature(x, y, RGB15(0,21,0));
    }
    if( xonix_cell_is(&c, PLAYER)) {
        draw_creature(x, y, RGB15(0,0,21));
    }
}

void process_changes()
{
    for( int i=0; i<p_cnt; i++) {
        draw_point(p+i);
    }
}

 
int main(void) 
{
	irqInit();
	irqEnable(IRQ_VBLANK);
 
	videoSetMode(MODE_FB0);
	vramSetBankA(VRAM_A_LCD);
 
    clear_screen();
    
    p_cnt = 0;
    xonix_init(32, 24, 17, 1, xonix_callback, 0);
    process_changes();
 
	//we like infinite loops in console dev!
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
	}
 
	return 0;
}

