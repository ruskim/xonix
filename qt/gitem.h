#ifndef GITEM_H
#define GITEM_H

#include <QGraphicsItem>
#include "engine/xonix.h"

class GItem : public QGraphicsItem
{
private:
    QColor m_color;
    ObjType m_type;
    int m_id;
    int m_x;
    int m_y;
    int m_new_x;
    int m_new_y;
    int m_old_x;
    int m_old_y;
    int m_t_len;
    int m_t_curr;
    bool m_transition;


public:
    GItem(XonixEvent *e);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget);
    QRectF boundingRect() const;
    void advance(int phase);
    void move(int x, int y, int t_len);
};

#endif // GITEM_H
