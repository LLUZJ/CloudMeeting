//数据接收模块

#include "recvsolve.h"
#include <QMetaType>                //管理元对象系统中的命名类型
#include <QDebug>
#include <QMutexLocker>
extern QUEUE_DATA<MESG> queue_recv;

void RecvSolve::stopImmediately()       //立刻停止 w199
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;                 //m_isCanRun可以运行 置false
}

RecvSolve::RecvSolve(QObject *par):QThread(par)     //接收解决
{
    qRegisterMetaType<MESG *>();
    m_isCanRun = true;                  //m_isCanRun可以运行 置true
}

void RecvSolve::run()                   //RecvSolve继承于QThread,重写run函数
{
    WRITE_LOG("start solving data thread: 0x%p", QThread::currentThreadId());       //日志"开始处理数据线程 ： 线程id"
    for(;;)
    {
        {
            QMutexLocker locker(&m_lock);
            if (m_isCanRun == false)            //如果m_isCanRun可以运行是false
            {
                WRITE_LOG("stop solving data thread: 0x%p", QThread::currentThreadId());        //日志"停止处理数据线程：线程id"
                return;
            }
        }
        MESG * msg = queue_recv.pop_msg();
        if(msg == NULL) continue;
		/*else free(msg);
		qDebug() << "取出队列:" << msg->msg_type;*/
        emit datarecv(msg);                     //发出datarecv数据接收信号，主窗口调用datasolve数据处理
    }
}
