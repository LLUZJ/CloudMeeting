//文字发送

#include "sendtext.h"
#include <QDebug>

extern QUEUE_DATA<MESG> queue_send;
#ifndef WAITSECONDS
#define WAITSECONDS 2
#endif
SendText::SendText(QObject *par):QThread(par)
{

}

SendText::~SendText()
{

}

void SendText::push_Text(MSG_TYPE msgType, QString str)     //w93
{
    //SendText放入到了_textThread线程中
    textqueue_lock.lock();                                  //上锁
    while(textqueue.size() > QUEUE_MAXSIZE)                 //textqueue队列大小 超过 队列最大值
    {
        queue_waitCond.wait(&textqueue_lock);
    }
    textqueue.push_back(M(str, msgType));                   //把M加到textqueue队列后面

    textqueue_lock.unlock();                                //解锁
    queue_waitCond.wakeOne();                               //唤醒添加线程
}


void SendText::run()
{
    m_isCanRun = true;                                      //m_isCanRun可以运行 置true
    WRITE_LOG("start sending text thread: 0x%p", QThread::currentThreadId());       //日志“开始发送文本线程 ：线程id”
    for(;;)
    {
        textqueue_lock.lock();                           //加锁62
        while(textqueue.size() == 0)
        {
            bool f = queue_waitCond.wait(&textqueue_lock, WAITSECONDS * 1000);      //阻塞线程
            if(f == false)                                  //timeout
            {
                QMutexLocker locker(&m_lock);
                if(m_isCanRun == false)
                {
                    textqueue_lock.unlock();
                    WRITE_LOG("stop sending text thread: 0x%p", QThread::currentThreadId());        //日志“停止发送文本线程：线程id”
                    return;
                }
            }
        }

        M text = textqueue.front();                         //textqueue的第一个元素 赋给M类的text

//        qDebug() << "取出队列:" << QThread::currentThreadId();

        textqueue.pop_front();                              //删除textqueue的第一个元素
        textqueue_lock.unlock();                         //解锁41
        queue_waitCond.wakeOne();                           //唤醒添加线程

        //构造消息体
        MESG* send = (MESG*)malloc(sizeof(MESG));
        if (send == NULL)
        {
            WRITE_LOG("malloc error");
            qDebug() << __FILE__  <<__LINE__ << "malloc fail";
            continue;
        }
        else
        {
            memset(send, 0, sizeof(MESG));                  //00000000通过复制的方式扩展到sizeof(MESG)字节，然后将这sizeof(MESG)字节根据数据类型分隔并从send指向的地址单元开始赋值，直到将全部字节用完

            if (text.type == CREATE_MEETING || text.type == CLOSE_CAMERA)   //如果text的类型为 创建会议 或者 关闭摄像头
			{
				send->len = 0;
				send->data = NULL;
                send->msg_type = text.type;                 //把text的类型 赋值给 消息类型
                queue_send.push_msg(send);                  //把send加入发送队列
			}
            else if (text.type == JOIN_MEETING)                             //如果text的类型为 加入会议
			{
                send->msg_type = JOIN_MEETING;              //把消息类型 设为JOIN_MEETING
                send->len = 4;                              //房间号占4个字节
                send->data = (uchar*)malloc(send->len + 10);
                
                if (send->data == NULL)
                {
                    WRITE_LOG("malloc error");
                    qDebug() << __FILE__ << __LINE__ << "malloc fail";
                    free(send);                             //释放send
                    continue;
                }
                else
                {
                    memset(send->data, 0, send->len + 10);      //00000000通过复制的方式扩展到send->len + 10字节，然后将这send->len + 10字节根据数据类型分隔并从send->data指向的地址单元开始赋值，直到将全部字节用完
					quint32 roomno = text.str.toUInt();
                    memcpy(send->data, &roomno, sizeof(roomno));        //复制字符串，把大小为sizeof(roomno)的&roomno复制给send->data
					//加入发送队列

					queue_send.push_msg(send);
                }
			}
            else if(text.type == TEXT_SEND)                                 //如果text的类型是 文本发送
            {
                send->msg_type = TEXT_SEND;                 //把消息类型 设为TEXT_SEND
                QByteArray data = qCompress(QByteArray::fromStdString(text.str.toStdString()));     //压缩
                send->len = data.size();                    //send所占字节为data.size
                send->data = (uchar *) malloc(send->len);
                if(send->data == NULL)
                {
                    WRITE_LOG("malloc error");
                    qDebug() << __FILE__ << __LINE__ << "malloc error";
                    free(send);                             //释放send
                    continue;
                }
                else
                {
                    memset(send->data, 0, send->len);       //00000000通过复制的方式扩展到send->len字节，然后将这send->len字节根据数据类型分隔并从send->data指向的地址单元开始赋值，直到将全部字节用完
                    memcpy_s(send->data, send->len, data.data(), data.size());      //把data.size长度的data.data复制，给send->data，准备一个send->len大小的缓冲区
                    queue_send.push_msg(send);              //把send 加入发送队列
                }
            }
        }
    }
}
void SendText::stopImmediately()            //立刻停止w223
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;                     //m_isCanRun可以运行 置false
}
