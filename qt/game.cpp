#include "game.h"
#include <QPainter>
#include <QPen>
#include <QKeyEvent>
#include <QDebug>

QPen freePen;
QBrush blockBrush(QColor(200,200,200));
QBrush bgBrush(QColor(255,255,255));

QMap<int,int> keyMap;

const int period=17;

void xonix_callback(void *tag, XonixEvent *e)
{
    if( tag) {
        ((Game *)tag)->xonixEvent(e);
    }
}

Game::Game(QObject *parent, int width, int height) :
    QGraphicsScene(parent),m_width(width),m_height(height)
{
    setSceneRect(0, 0, m_width, m_height);

    freePen.setColor(QColor(200,200,200));
    freePen.setWidth(0);

    keyMap[Qt::Key_Up] = X_KEY_UP;
    keyMap[Qt::Key_Down] = X_KEY_DOWN;
    keyMap[Qt::Key_Left] = X_KEY_LEFT;
    keyMap[Qt::Key_Right] = X_KEY_RIGHT;
    m_evils = 0;

    m_key_mask = 0;
    m_timer.setInterval(period);
    QObject::connect(&m_timer, SIGNAL(timeout()), this, SLOT(myAdvance()));

    //startGame();
}

void Game::startGame()
{
    QList<int> keys = m_items.keys();
    foreach(int k, keys) {
        GItem *i = m_items[k];
        removeItem(i);
        delete i;
    }
    m_evils++;
    m_key_mask = 0;
    m_victory = false;
    pressedKeys.clear();
    invalidate(sceneRect(), QGraphicsScene::BackgroundLayer);
    xonix_free();
    xonix_init(m_width, m_height, period, m_evils, xonix_callback, this);
    m_timer.start();
}

void Game::drawBackground ( QPainter * painter, const QRectF & rect )
{
    painter->setPen(freePen);
    //qDebug() << "rect: " << rect;
    painter->setRenderHint(QPainter::Antialiasing);
    for(int x=0; x<m_width; x++) {
        for(int y=0; y<m_height; y++) {
            bool busy=false;
            Cell c;
            xonix_get_cell(x, y, &c);
            if( xonix_cell_is(&c, BLOCK)) {
                busy=true;
            }
            drawBlock(busy, x, y, painter, rect);
        }
    }
    if( m_victory) {
        QBrush textBrush(QColor(0,220,0));
        QPen textPen;
        textPen.setWidth(0);
        QFont font;

        textPen.setColor(QColor(0,220,0));
        painter->setBrush(textBrush);
        painter->setPen(textPen);
        QRectF r=sceneRect();

        font = painter->font();

        font.setFamily("Arial");

        font.setPointSizeF(1.0*m_width/15);
        painter->setFont(font);

        painter->drawText(0,0, m_width, m_height/1.5, Qt::AlignCenter, "Level  cleared");

        font.setPointSizeF(1.0*m_width/30);
        painter->setFont(font);

        painter->drawText(0,0, m_width, m_height*1.1, Qt::AlignCenter, "Press space to continue");
    }
}

void Game::drawBlock(bool busy, int ix, int iy, QPainter *painter, const QRectF & rect)
{
    qreal x = ix;
    qreal y = iy;
    qreal m = 0.05;
    QRectF r(x+m, y+m, 1-2*m, 1-2*m);
    QRectF cell_rect(x, y, 1, 1);

    if( !cell_rect.intersects(rect)) {
        return;
    }

    painter->fillRect(QRectF(x, y, 1, 1), bgBrush);

    if(busy) {
        painter->fillRect(r, blockBrush);
    } else {
        painter->drawRect(r);
    }

}

void Game::keyPressEvent( QKeyEvent *keyEvent)
{
    int k = keyEvent->key();
    if( m_victory) {
        if( k == Qt::Key_Space) {
            startGame();
        }
    }
    pressedKeys.insert(k);
    if(keyMap.contains(k)) {
        m_key_mask |= keyMap[k];
    }
}

void Game::keyReleaseEvent( QKeyEvent * keyEvent)
{
    pressedKeys.remove(keyEvent->key());
}

void Game::focusInEvent ( QFocusEvent * )
{
    pressedKeys.clear();
    m_timer.start();
}

void Game::focusOutEvent( QFocusEvent * )
{
    pressedKeys.clear();
    m_timer.stop();
}

void Game::myAdvance()
{

    foreach(int k, pressedKeys){
        if(keyMap.contains(k)) {
            m_key_mask |= keyMap[k];
        }
    }

    m_blocks_removed = false;
    xonix_advance(m_key_mask);
    m_key_mask = 0;
    if(m_blocks_removed) {
        invalidate(sceneRect(), QGraphicsScene::BackgroundLayer);
    }

    advance();
}

void Game::victory()
{
    m_victory = true;
    m_key_mask = 0;
    pressedKeys.clear();
    invalidate(sceneRect(), QGraphicsScene::BackgroundLayer);
}

void Game::xonixEvent(XonixEvent *e)
{
    if(e->et == eVictory) {
        victory();
        return;
    }

    if(e->et == eScore) {
        emit score(e->x);
    }

    if(e->et == eObjAdded) {
        if(e->ot == objBlock) {
            return;
        }
        GItem *i = new GItem(e);
        addItem(i);
        m_items[e->id] = i;
    }

    if(e->et == eObjMoved) {
        (m_items[e->id])->move(e->x, e->y, e->t_len);
    }

    if(e->et == eObjDeleted) {
        if(e->ot == objBlock) {
            m_blocks_removed = true;
            //victory();
            return;
        }

        GItem *i = m_items[e->id];
        m_items.remove(e->id);
        removeItem(i);
        delete i;
    }
}
