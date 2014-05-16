#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::setScene(QGraphicsScene *scene)
{
    ui->graphicsView->setCacheMode(QGraphicsView::CacheBackground);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->fitInView(scene->sceneRect());
}

void Widget::score(int s)
{
    ui->lScore->setText(QString::number(100-s) + "%");
}
