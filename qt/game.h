#ifndef GAME_H
#define GAME_H

#include <QGraphicsScene>
#include <QTimer>
#include <QSet>
#include "gitem.h"
#include "engine/xonix.h"

class Game : public QGraphicsScene
{
    Q_OBJECT
private:
    int m_width;
    int m_height;
    QTimer m_timer;
    QSet<int> pressedKeys;
    QMap<int,GItem*> m_items;
    bool m_blocks_removed;
    int m_key_mask;
    bool m_victory;
    int m_evils;

    void drawBlock(bool busy, int x, int y, QPainter *painter, const QRectF & rect );
    void drawBackground ( QPainter * painter, const QRectF & rect );
    void victory();


    void keyPressEvent( QKeyEvent *keyEvent);
    void keyReleaseEvent( QKeyEvent * keyEvent);
    void focusInEvent ( QFocusEvent * focusEvent);
    void focusOutEvent ( QFocusEvent * focusEvent);

public:
    explicit Game(QObject *parent = 0, int width=40, int height=30);
    void startGame();
    void xonixEvent( XonixEvent *e);

signals:
    void score(int s);

public slots:
    void myAdvance();


};

#endif // GAME_H
