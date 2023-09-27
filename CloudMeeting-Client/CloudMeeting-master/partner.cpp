//房间信息

#include "partner.h"
#include <QDebug>
#include <QEvent>
#include <QHostAddress>
Partner::Partner(QWidget *parent, quint32 p):QLabel(parent)
{
//    qDebug() <<"dsaf" << this->parent();
    ip = p;

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);      //设置大小策略（合理大小允许缩放 ，有最小值）
    w = ((QWidget *)this->parent())->size().width();
    this->setPixmap(QPixmap::fromImage(QImage(":/myImage/1.jpg").scaled(w-10, w-10)));
    this->setFrameShape(QFrame::Box);                                       //设置控件窗体类型：长方形盒子

    this->setStyleSheet("border-width: 1px; border-style: solid; border-color:rgba(0, 0 , 255, 0.7)");
//    this->setToolTipDuration(5);

    this->setToolTip(QHostAddress(ip).toString());
}


void Partner::mousePressEvent(QMouseEvent *)            //鼠标按压事件
{
    emit sendip(ip);        //发送信号sendip发送ip,主窗口调用recvip接收ip    w654,678
}
void Partner::setpic(QImage img)        //设置图片  w179,372,554,593
{
    this->setPixmap(QPixmap::fromImage(img.scaled(w-10, w-10)));
}
