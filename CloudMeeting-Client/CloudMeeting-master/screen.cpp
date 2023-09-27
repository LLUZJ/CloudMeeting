//屏幕工具模块

#include "screen.h"
#include <QGuiApplication>          //管理GUI应用程序的控制流和主设置
#include <QApplication>             //在 Qt Widgets应用程序中管理GUI程序的控制流和主要设置
#include <QScreen>                  //获取系统屏幕大小
#include <QDebug>

int Screen::width = -1;
int Screen::height = -1;


void Screen::init()                 //屏幕工具初始化
{
    QScreen *s = QGuiApplication::primaryScreen();      //s:初始化屏幕


    Screen::width = s->geometry().width();              //geometry是一种用于管理窗口控件位置和大小的重要方法,坐标系统
    Screen::height = s->geometry().height();

//    qDebug() << Screen::width << " " << Screen::height;
}
