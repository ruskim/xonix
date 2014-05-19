#include "xonix.h"
#include "malloc.h"
#include "string.h"

typedef unsigned short Cell;

#define NET   0x10



typedef struct tagPos
{
    unsigned char x;
    unsigned char y;
} Pos;

typedef struct tagIPos
{
    int x;
    int y;
} IPos;

typedef struct tagEvil
{
    IPos p;
    IPos t[2]; //move trace
    int tlen;
    int vx;
    int x_period;
    int x_phase;
    int vy;
    int y_period;
    int y_phase;
} Evil;

typedef struct tagXonix
{
    int w; //width
    int h; //height
    Cell *field;
    int px; //player_x
    int py; //player_y
    int p_period; //player period
    int period; //timer period in msec
    int ticks;  //timer ticks
    EventCallback callback;
    void *event_tag;
    Pos *path;
    int path_len;
    int path_max_len;
    int respawn;
    Evil *evils;
    int evils_cnt;
    int key_mask;
    int total_block;
    int curr_block;
    int score;
} Xonix;

static Xonix *game = 0;

static int shift(int x, int y)
{
    return y*game->w + x;
}


static void cbrun( XonixEvent* e)
{
    if( game->callback) {
        game->callback(game->event_tag, e);
    }
}

static void update_score()
{
    XonixEvent e;
    game->score = 100*game->curr_block/game->total_block;
    e.et = eScore;
    e.x = game->score;
    cbrun(&e);
}

static int cell_is(Cell *c, Cell mask)
{
    return (*c) & mask;
}

int xonix_cell_is(Cell *c, Cell mask)
{
    return cell_is(c, mask);
}


static void cell_set(Cell *c, Cell mask)
{
    (*c) |= mask;
}

static void cell_clear(Cell *c, Cell mask)
{
    (*c) &= ~(mask);
}

static Cell* get_cell_ptr(int x, int y)
{
    return game->field + shift(x,y);
}

void xonix_get_cell(int x, int y, Cell *c)
{
    *c = *get_cell_ptr(x, y);
}

static void set_block(int x, int y)
{
    XonixEvent e;

    cell_set(get_cell_ptr(x, y), BLOCK);

    e.et = eObjAdded;
    e.ot = objBlock;
    e.id = BLOCK_ID_SHIFT + shift(x, y);
    e.x = x;
    e.y = y;
    cbrun(&e);
}

static void add_player(int x, int y)
{
    XonixEvent e;
    Cell *c;

    game->px = x;
    game->py = y;
    game->respawn = 0;
    c = get_cell_ptr(x, y);
    cell_set(c, PLAYER);

    e.et = eObjAdded;
    e.ot = objPlayer;
    e.id = PLAYER_ID;
    e.x = x;
    e.y = y;
    cbrun(&e);
}

static void del_player()
{
    XonixEvent e;
    Cell *c;

    c = get_cell_ptr(game->px, game->py);
    cell_clear(c, PLAYER);

    game->respawn = 1;
    e.et = eObjDeleted;
    e.ot = objPlayer;
    e.id = 0;
    e.x = game->px;
    e.y = game->py;
    cbrun(&e);
}

static void path_push(x, y)
{
    Pos *p;

    if(game->path_len == game->path_max_len) {
        game->path_max_len += game->w;
        game->path = realloc(game->path, game->path_max_len*sizeof(Pos));
    }
    p = game->path + game->path_len;
    p->x = x;
    p->y = y;
    (game->path_len)++;
}

static int path_pop( int *x, int *y)
{
    Pos *p;
    if( game->path_len == 0)
        return -1;

    (game->path_len)--;
    p = game->path + game->path_len;
    *x = p->x;
    *y = p->y;

    return 0;
}

static void del_block(Cell *c, int x, int y)
{
    XonixEvent e;

    e.et = eObjDeleted;
    e.ot = objBlock;
    e.x = x;
    e.y = y;
    e.id = BLOCK_ID_SHIFT + shift(x, y);
    cell_clear(c, BLOCK);
    game->curr_block--;
    cbrun(&e);

}

static int check_cell(Cell *c) {
    return cell_is(c, BLOCK) && (!cell_is(c, NET));
}

static void proc_cell(Cell *c, int x, int y)
{
    Cell *c_up, *c_down;

    cell_set(c, NET);
    if( y > 0) {
        c_up = c - game->w;
        if( check_cell(c_up)) {
            path_push(x, y-1);
        }
    }

    if(y<(game->h-1)) {
        c_down = c + game->w;
        if( check_cell(c_down)) {
            path_push(x, y+1);
        }
    }
}

static void fill_line(int x0, int y0)
{
    Cell *c0, *c_w, *c_e, *c;
    int        x_w,  x_e,  x;

    c0 = get_cell_ptr(x0, y0);
    if(!check_cell(c0)) {
        return;
    }

    c = c0;
    for(x=x0; x>=0; x--) {
        if( !check_cell(c)) {
            break;
        }
        c_w = c;
        x_w = x;
        c -= 1;
    }

    c = c0;
    for(x=x0;x<game->w;x++) {
        if( !check_cell(c)) {
            break;
        }
        c_e = c;
        x_e = x;
        c += 1;
    }

    c = c_w;
    for(x=x_w; x<=x_e; x++) {
        proc_cell(c, x, y0);
        c += 1;
    }
}

static void sweep_blocks()
{
    int i;
    int x,y;

    for(i=0; i<game->evils_cnt; i++) {
        Evil *e = game->evils + i;
        path_push(e->p.x, e->p.y);
    }

    while(game->path_len) {
        int x, y;
        path_pop(&x, &y);
        fill_line(x, y);
    }

    for(x=0; x<game->w; x++) {
        for(y=0; y<game->h; y++) {
            Cell *c = get_cell_ptr(x, y);
            if( check_cell(c)) {
                del_block(c, x, y);
            }
            cell_clear(c, NET);
        }
    }
}

static void del_path(int do_sweep_blocks)
{
    XonixEvent e;

    e.et = eObjDeleted;
    e.ot = objPath;
    while(game->path_len) {
        Cell *c;
        int x, y;

        path_pop(&x, &y);
        c = get_cell_ptr(x, y);
        cell_clear(c, PATH);
        e.id = PATH_ID_SHIFT + game->path_len;
        e.x = x;
        e.y = y;
        cbrun(&e);
        if( do_sweep_blocks) {
            del_block(c, x, y);
        }
    }

    if(do_sweep_blocks) {
        sweep_blocks();
        update_score();
        if( game->score <= 25) {
            e.et = eVictory;
            cbrun(&e);
        }
    }
}

static void add_to_path(Cell *c, int x, int y)
{
    XonixEvent e;

    e.et = eObjAdded;
    e.ot = objPath;
    e.id = game->path_len + PATH_ID_SHIFT;
    e.x = x;
    e.y = y;
    e.t_len = game->p_period;

    path_push(x, y);
    cell_set(c, PATH);

    cbrun(&e);

}

static void move_player(int key_mask)
{
    int dx=0, dy=0;
    int x, y;
    int prev_x, prev_y;
    XonixEvent e;
    Cell *c;

    game->key_mask |= key_mask;

    if( game->ticks % game->p_period != 0) {
        return;
    }

    key_mask = game->key_mask;
    game->key_mask = 0;

    if( key_mask & X_KEY_DOWN) {
        dy = 1;
    } else if( key_mask & X_KEY_UP) {
        dy = -1;
    } else if( key_mask & X_KEY_LEFT) {
        dx = -1;
    } else if( key_mask & X_KEY_RIGHT) {
        dx = 1;
    } else {
        return;
    }

    prev_x = game->px;
    prev_y = game->py;
    x = game->px+dx;
    y = game->py+dy;

    if( x < 0 || y < 0 || x >= game->w || y >= game->h)
        return;

    game->px = x;
    game->py = y;

    c = get_cell_ptr(prev_x, prev_y);
    cell_clear(c, PLAYER);

    e.et = eObjMoved;
    e.ot = objPlayer;
    e.id = 0;
    e.x = game->px;
    e.y = game->py;
    e.prev_x = prev_x;
    e.prev_y = prev_y;
    e.t_len = game->p_period;
    cbrun(&e);

    c = get_cell_ptr(x,y);
    if(cell_is(c, PATH)) {
        del_path(0);
        del_player();
        return;
    }

    if(cell_is(c, BLOCK)) {
        add_to_path(c, game->px, game->py);
    } else {
        if( game->path_len) {
            del_path(1);
        }
    }
    if(cell_is(c, EVIL)) {
        del_path(0);
        del_player();
        return;
    }
    cell_set(c, PLAYER);
}

static int check_evil_move(IPos *p)
{
    Cell *c;

    if( (p->x < 0) || (p->x >= game->w) || (p->y < 0) || (p->y >= game->h)) {
        return 0;
    }

    c = get_cell_ptr(p->x, p->y);
    if( !cell_is(c, BLOCK)) {
        return 0;
    }

    return 1;
}

static void move_evils()
{
    int i;
    XonixEvent e;
    IPos *p, *p_prev;
    Evil *ev;
    Cell *c;

    e.et = eObjMoved;
    e.ot = objEvil;

    for(i=0;i<game->evils_cnt;i++) {
        ev = game->evils + i;
        c = get_cell_ptr(ev->p.x, ev->p.y);
        cell_clear(c, EVIL);
        ev->tlen = 0;
        if( game->ticks % ev->x_period == ev->x_phase) {
            p = ev->t + ev->tlen;
            p_prev = &ev->p;
            p->x = p_prev->x + ev->vx;
            p->y = p_prev->y;
            if( check_evil_move(p)) {
                (ev->tlen)++;
            } else {
                ev->vx = -ev->vx;
            }
        }
        if( game->ticks % ev->y_period == ev->y_phase) {
            p = ev->t + ev->tlen;
            if( ev->tlen) {
                p_prev = ev->t + 0;
            } else {
                p_prev = &ev->p;
            }
            p->x = p_prev->x;
            p->y = p_prev->y + ev->vy;
            if( check_evil_move(p)) {
                (ev->tlen)++;
            } else {
                ev->vy = -ev->vy;
            }
        }
    }

    for(i=0;i<game->evils_cnt;i++) {
        int j;
        ev = game->evils + i;
        e.prev_x = ev->p.x;
        e.prev_y = ev->p.y;
        for(j=0;j<ev->tlen;j++) {
            p = ev->t + j;

            if( j == (ev->tlen-1)) {
                //notify only about last point
                int tx, ty;

                e.x = ev->p.x = p->x;
                e.y = ev->p.y = p->y;
                e.id = EVIL_ID_SHIFT + i;

                tx = ev->x_period - (game->ticks%ev->x_period);
                ty = ev->y_period - (game->ticks%ev->y_period);
                e.t_len = (tx < ty) ? tx : ty;

                cbrun(&e);
            }

            c = get_cell_ptr(p->x, p->y);
            if( cell_is(c, PATH)) {
                del_path(0);
                del_player();
            }
        }
    }

    for(i=0;i<game->evils_cnt;i++) {
        ev = game->evils + i;
        c = get_cell_ptr(ev->p.x, ev->p.y);
        cell_set(c, EVIL);
    }
}

//int xonix_is_block( int x, int y)
//{
//    return cell_is( get_cell_ptr(x, y), BLOCK);
//}

void xonix_advance(int key_mask)
{

    if( game->respawn) {
        add_player(0,0);
    } else {
        move_player(key_mask);
    }

    move_evils();
    game->ticks++;
}

void add_evil(int x, int y, int vx, int vy)
{
    Evil *ev;
    XonixEvent e;

    e.id = EVIL_ID_SHIFT + game->evils_cnt;
    game->evils = realloc(game->evils, (game->evils_cnt + 1)*sizeof(Evil));
    ev = game->evils + game->evils_cnt;
    (game->evils_cnt)++;
    ev->p.x = x;
    ev->p.y = y;

    ev->vx = vx;
    ev->x_period = 10;
    ev->x_phase = 0;

    ev->vy = vy;
    ev->y_phase = 0;
    ev->y_period = ev->x_period;

    e.et = eObjAdded;
    e.ot = objEvil;
    e.x = x;
    e.y = y;

    cell_set(get_cell_ptr(x,y), EVIL);

    cbrun(&e);
}


void xonix_init( int w, int h, int period, int evils_cnt, EventCallback callback, void *event_tag)
{
    int fs, i, j;

    int v[4][2] = {
        {1, -1},
        {-1, 1},
        {1, 1},
        {-1,-1}
    };

    int c[4][4] = {
        {0, 0},
        {1, 1},
        {1, 0},
        {0, 1}
    };

    game = malloc(sizeof(Xonix));
    memset(game, 0, sizeof(Xonix));

    game->w = w;
    game->h = h;
    game->period = period;
    fs = w*h*sizeof(game->field[0]);
    game->field = malloc(fs);
    memset(game->field, 0, fs);
    game->px = 0;
    game->py = 0;
    game->ticks = 0;
    game->callback = callback;
    game->event_tag = event_tag;

    game->respawn = 0;
    game->p_period = 5;
    game->key_mask = 0;

    game->total_block = 0;
    game->curr_block = 0;

    add_player(0,0);

    for( i=0; i<evils_cnt; i++) {
        int d = ((int)(i/4) + 1) * 4;
        int n = i%4;
        int x, y;

        x = c[n][0] ? w-d : d;
        y = c[n][1] ? h-d : d;

        add_evil(x, y, v[n][0], v[n][1]);
    }

    for(i=1; i<w-1; i++) {
        for(j=1; j<h-1; j++) {
            set_block(i,j);
            game->total_block++;
        }
    }
    game->curr_block = game->total_block;
    update_score();
}

void xonix_free()
{
    if( game) {
        free(game->path);
        free(game->evils);
        free(game);
    }
    game = 0;
}
