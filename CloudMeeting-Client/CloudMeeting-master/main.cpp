#include <QApplication>
#include "widget.h"                 //主窗口
#include "screen.h"                 //屏幕工具模块
#include <QTextCodec>               //编码转换
int main(int argc, char* argv[])
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF8"));
    QApplication app(argc, argv);
    Screen::init();

    Widget w;
    w.show();
    return app.exec();
}
