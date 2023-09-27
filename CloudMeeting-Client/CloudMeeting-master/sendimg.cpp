//图片发送

#include "sendimg.h"
#include "netheader.h"          //网络包模块
#include <QDebug>
#include <cstring>              //提供各种丰富的操作函数、操作符重载，使我们使用起串起来更象basic中那样直观；而且它还提供了动态内存分配，使我们减少了多少字符串数组越界的隐患
#include <QBuffer>              //缓冲区类

extern QUEUE_DATA<MESG> queue_send;

SendImg::SendImg(QObject *par):QThread(par)
{

}

//消费线程
void SendImg::run()             //SendImg继承于QThread,重写run函数
{
    WRITE_LOG("start sending picture thread: 0x%p", QThread::currentThreadId());        //日志"开始发送图像线程：线程id"
    m_isCanRun = true;          //m_isCanRun可以运行 置true
    for(;;)
    {
        queue_lock.lock();              //加锁 44

        while(imgqueue.size() == 0)         //imgqueue：QByteArray类型的队列
        {
            //qDebug() << this << QThread::currentThreadId();
            bool f = queue_waitCond.wait(&queue_lock, WAITSECONDS * 1000);
            if (f == false)                 //timeout
			{
				QMutexLocker locker(&m_lock);
                if (m_isCanRun == false)            //如果m_isCanRun可以运行 为false
				{
                    queue_lock.unlock();            //暂时解锁23
                    WRITE_LOG("stop sending picture thread: 0x%p", QThread::currentThreadId());     //日志“停止发送图片线程：线程id”
					return;
				}
			}
        }

        QByteArray img = imgqueue.front();
//        qDebug() << "取出队列:" << QThread::currentThreadId();
        imgqueue.pop_front();            //删除imgqueue中的第一个位置的元素
        queue_lock.unlock();             //解锁 23
        queue_waitCond.wakeOne();       //唤醒添加线程


        //构造消息体
        MESG* imgsend = (MESG*)malloc(sizeof(MESG));
        if (imgsend == NULL)
        {
            WRITE_LOG("malloc error");          //日志“malloc错误”
            qDebug() << "malloc imgsend fail";  //打印malloc 照片发送失败
        }
        else
        {
            memset(imgsend, 0, sizeof(MESG));       //00000000通过复制的方式扩展到sizeof(MESG)字节，然后将这sizeof(MESG)字节根据数据类型分隔并从imgsend指向的地址单元开始赋值，直到将全部字节用完
            imgsend->msg_type = IMG_SEND;           //图像发送
            imgsend->len = img.size();
            qDebug() << "img size :" << img.size(); //打印“图像大小：xx”
            imgsend->data = (uchar*)malloc(imgsend->len);
            if (imgsend->data == nullptr)
            {
                free(imgsend);                      //释放掉imgsend
                WRITE_LOG("malloc error");          //日志“malloc错误”
                qDebug() << "send img error";       //打印“发送图像错误”
                continue;
            }
            else
            {
                memset(imgsend->data, 0, imgsend->len);         //00000000通过复制的方式扩展到imgsend->len字节，然后将这imgsend->len字节根据数据类型分隔并从imgsend->data指向的地址单元开始赋值，直到将全部字节用完
                memcpy_s(imgsend->data, imgsend->len, img.data(), img.size());      //imgsend->data拷贝完成之后的字符串，imgsend->len拷贝完成目标缓冲区长度，img.data需要拷贝的字符串，img.size需要拷贝字符串长度      memcpy_s对字符串就行拷贝
				//加入发送队列
				queue_send.push_msg(imgsend);
            }
        }
    }
}

//添加线程
void SendImg::pushToQueue(QImage img)       //推入到队列     103
{
    //压缩
    QByteArray byte;
    QBuffer buf(&byte);
    buf.open(QIODevice::WriteOnly);         //只写方式打开
    img.save(&buf, "JPEG");                 //图像保存
    QByteArray ss = qCompress(byte);        //调用压缩byte
    QByteArray vv = ss.toBase64();          //对数据编解码
//    qDebug() << "加入队列:" << QThread::currentThreadId();
    queue_lock.lock();                          //上锁97
    while(imgqueue.size() > QUEUE_MAXSIZE)      //如果imgqueue的大小 超过 队列最大值
    {
        queue_waitCond.wait(&queue_lock);
    }
    imgqueue.push_back(vv);                     //把编解码处理好的数据vv 插入imgqueue队列尾
    queue_lock.unlock();                        //解锁91
    queue_waitCond.wakeOne();                   //唤醒添加线程
}

void SendImg::ImageCapture(QImage img)      //w105
{
    pushToQueue(img);                       //把img 推入到队列
}

void SendImg::clearImgQueue()               //w109
{
    qDebug() << "清空视频队列";
    QMutexLocker locker(&queue_lock);
    imgqueue.clear();                       //清空imgqueue队列
}


void SendImg::stopImmediately()             //w211
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;                     //m_isCanRun可以运行 置false
}
