#include "gitem.h"
#include <QBrush>
#include <QPainter>

GItem::GItem( XonixEvent *e)
{
    m_type = e->ot;
    m_id = e->id;
    m_x = e->x;
    m_y = e->y;
    m_new_x = -1;
    m_new_y = -1;
    m_t_curr = 0;
    m_t_len = 0;
    switch(m_type) {

    case objPlayer:
        m_color = QColor(30,144,255);
        setZValue(1.0);
        break;

    case objEvil:
        m_color = QColor(0,0,0);
        break;

    case objPath:
        m_t_curr = 0;
        m_t_len = e->t_len;
        setZValue(0.5);
        m_color = QColor(255,215,0);
    }

    setPos(m_x, m_y);
}

QRectF GItem::boundingRect() const
{
    qreal adjust = 0.05;
    return QRectF( -adjust, -adjust, 1+2*adjust, 1+2*adjust);
}

void GItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if( m_type == objPath) {
        if( m_t_curr != m_t_len) {
            return;
        }
    }

    QBrush b = painter->brush();
    QPen p = painter->pen();
    p.setStyle(Qt::NoPen);
    painter->setPen(p);

    b.setColor(this->m_color);
    b.setStyle(Qt::SolidPattern);
    painter->setBrush(b);
    painter->drawEllipse(QPointF(0.5,0.5), 0.3, 0.3);
}

void GItem::move(int x, int y, int t_len)
{
    m_old_x = m_x;
    m_old_y = m_y;
    m_x = x;
    m_y = y;
    m_t_len = t_len;
    m_t_curr = 0;
}

void GItem::advance(int phase)
{
    if(phase == 0) {
        return;
    }

    if( m_t_len == m_t_curr) {
        return;
    }

    m_t_curr++;

    if(m_type == objPath) {
        return;
    }

    qreal x = 1.0*m_x*m_t_curr/m_t_len + 1.0*(m_old_x)*(m_t_len - m_t_curr)/m_t_len;
    qreal y = 1.0*m_y*m_t_curr/m_t_len + 1.0*(m_old_y)*(m_t_len - m_t_curr)/m_t_len;
    setPos(x, y);
}
