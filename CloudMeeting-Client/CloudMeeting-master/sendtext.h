#ifndef SENDTEXT_H
#define SENDTEXT_H

//文字发送
#include <QThread>
#include <QMutex>
#include <QQueue>
#include "netheader.h"      //网络包模块

struct M
{
    QString str;
    MSG_TYPE type;

    M(QString s, MSG_TYPE e)
    {
        str = s;
        type = e;
    }
};

//发送文本数据
class SendText : public QThread         //w87
{
    Q_OBJECT
private:
    QQueue<M> textqueue;
    QMutex textqueue_lock; //队列锁
    QWaitCondition queue_waitCond;
    void run() override;
    QMutex m_lock;
    bool m_isCanRun;
public:
    SendText(QObject *par = NULL);
    ~SendText();
public slots:
    void push_Text(MSG_TYPE, QString str = "");     //w93
    void stopImmediately();                         //w223
};

#endif // SENDTEXT_H
