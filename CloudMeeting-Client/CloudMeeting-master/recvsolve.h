#ifndef RECVSOLVE_H
#define RECVSOLVE_H

//数据接收模块
#include "netheader.h"          //网络包模块
#include <QThread>
#include <QMutex>
/*
 * 接收线程
 * 从接收队列拿取数据
 */
class RecvSolve : public QThread            //w113
{
    Q_OBJECT
public:
    RecvSolve(QObject *par = NULL);
    void run() override;
private:
    QMutex m_lock;
    bool m_isCanRun;
signals:
    void datarecv(MESG *);          //w114
public slots:
    void stopImmediately();         //w199
};

#endif // RECVSOLVE_H
