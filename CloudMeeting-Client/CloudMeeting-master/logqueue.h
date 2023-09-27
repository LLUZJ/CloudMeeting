#ifndef LOGQUEUE_H
#define LOGQUEUE_H

//日志队列
#include <QThread>                  //线程
#include <QMutex>                   //互斥锁
#include <queue>                    //queue队列
#include "netheader.h"          //网络包模块

class LogQueue : public QThread     //w35
{
private:
    void run();
    QMutex m_lock;
    bool m_isCanRun;

    QUEUE_DATA<Log> log_queue;
    FILE *logfile;                  //日志文件
public:
    explicit LogQueue(QObject *parent = nullptr);
    void stopImmediately();         //w243
    void pushLog(Log*);
};

#endif // LOGQUEUE_H
