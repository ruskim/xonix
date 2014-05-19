#ifndef XONIX_H
#define XONIX_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short Cell;

#define BLOCK    0x1
#define PATH     0x2
#define EVIL     0x4
#define PLAYER   0x8

#define X_KEY_UP    0x1
#define X_KEY_DOWN  0x2
#define X_KEY_LEFT  0x4
#define X_KEY_RIGHT 0x8

#define PLAYER_ID      0x0
#define PATH_ID_SHIFT  0x400
#define BLOCK_ID_SHIFT 0x8000
#define EVIL_ID_SHIFT  0x10

typedef enum {
    eObjAdded,
    eObjMoved,
    eObjDeleted,
    eVictory,
    eScore
} EventType;

typedef enum {
    objPlayer,
    objEvil,
    objBlock,
    objPath
} ObjType;

typedef struct tagXonixEvent
{
    EventType et;
    ObjType ot; //object type
    int id; //object id
    int x;
    int y;
    int prev_x;
    int prev_y;
    int t_len; //transition length
} XonixEvent;

typedef void (*EventCallback)(void *tag, XonixEvent* e);

void xonix_init( int w, int h, int period, int evils, EventCallback callback, void* event_tag);
void xonix_advance(int key_mask);
void xonix_get_cell(int x, int y, Cell *c);
int xonix_cell_is(Cell *c, Cell mask);
void xonix_free();

#ifdef __cplusplus
}
#endif

#endif // XONIX_H
