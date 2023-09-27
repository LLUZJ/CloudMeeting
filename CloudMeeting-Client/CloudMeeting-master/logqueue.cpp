//日志队列

#include "logqueue.h"
#include <QDebug>

LogQueue::LogQueue(QObject *parent) : QThread(parent)
{

}

void LogQueue::pushLog(Log* log)
{
    log_queue.push_msg(log);
}

void LogQueue::run()            //LogQueue继承与QThread 重写run方法
{
    m_isCanRun = true;          //m_isCanRun可以运行，置true
    for(;;)
    {
        {
            QMutexLocker lock(&m_lock);
            if(m_isCanRun == false)         //如果m_isCanRun 为 false
            {
                fclose(logfile);            //关闭日志文件流； fclose:关闭一个流，使用fclose函数就可以把缓冲区内最后剩余的数据输出到内核缓冲区，并释放文件指针和有关的缓冲区
                return;
            }
        }
        Log *log = log_queue.pop_msg();     //?
        if(log == NULL || log->ptr == NULL) continue;


        //----------------write to logfile-------------------写入日志文件
        errno_t r = fopen_s(&logfile, "./log.txt", "a");            //使用errno_t fopen_s方式打开日志文件；    单独使用fopen_s时编译器会报错
        if(r != 0)
        {
            qDebug() << "打开文件失败:" << r;
            continue;
        }


        qint64 hastowrite = log->len;
        qint64 ret = 0, haswrite = 0;
        while ((ret = fwrite( (char*)log->ptr + haswrite, 1 ,hastowrite - haswrite, logfile)) < hastowrite)     //fwrite读取文件数据（指针ptr指向要写出数据的内存首地址 , 要写出数据的基本单元的字节大小即写出单位的大小1 ,  要写出数据的基本单元的个数(hastowrite - haswrite) , 打开的文件指针logfile）
        {
            if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) )
            {
                ret = 0;
            }
            else
            {
                qDebug() << "write logfile error";                  //"写文件日志错误"
                break;
            }
            haswrite += ret;
            hastowrite -= ret;
        }

        //free
        if(log->ptr) free(log->ptr);
        if(log) free(log);

        fflush(logfile);                //更新日志文件缓存区
        fclose(logfile);                //关闭日志文件流
    }
}

void LogQueue::stopImmediately()        //w243
{
    QMutexLocker lock(&m_lock);
    m_isCanRun = false;                 //m_isCanRun 置为 false
}
