#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QGraphicsScene>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    void setScene( QGraphicsScene *scene);

public slots:
    void score(int s);

private:
    Ui::Widget *ui;
};

#endif // WIDGET_H
