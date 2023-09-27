//网络包模块

#include "netheader.h"
#include "logqueue.h"           //日志队列
#include <QDebug>
#include <time.h>               //定义了四个变量类型、两个宏和各种操作日期和时间的函数

QUEUE_DATA<MESG> queue_send; //�ı�����Ƶ���Ͷ���      AI97;   mts174;     si74;   st82，104，124
QUEUE_DATA<MESG> queue_recv; //���ն���              mts67，269;  rs34
QUEUE_DATA<MESG> audio_recv; //��Ƶ���ն���           AO125,173

LogQueue *logqueue = nullptr;       //w26,35,36,241

void log_print(const char *filename, const char *funcname, int line, const char *fmt, ...)      //日志打印
{
    Log *log = (Log *) malloc(sizeof (Log));
    if(log == nullptr)
    {
        qDebug() << "malloc log fail";          //"malloc 多态分配 日志 失败"
    }
    else
    {
        memset(log, 0, sizeof (Log));           //以字节单元赋值，将0表示的00000000，通过复制的方式扩展至sizeof (Log)字节，然后将这sizeof (Log)根据数据类型分隔并从log指向的地址单元开始赋值，直到将全部字节用完
        log->ptr = (char *) malloc(1 * KB);
        if(log->ptr == nullptr)
        {
            free(log);                          //释放log
            qDebug() << "malloc log.ptr fail";  //打印“malloc多态分配 日志ptr 失败”
            return;
        }
        else
        {
            memset(log->ptr, 0, 1 * KB);
            time_t t = time(NULL);
            int pos = 0;
            int m = strftime(log->ptr + pos, KB - 2 - pos, "%F %H:%M:%S ", localtime(&t));
			pos += m;

            m = snprintf(log->ptr + pos, KB - 2 - pos, "%s:%s::%d>>>", filename, funcname, line);
			pos += m;

			va_list ap;
			va_start(ap, fmt);
            m = _vsnprintf(log->ptr + pos, KB - 2 - pos, fmt, ap);
			pos += m;
			va_end(ap);
            strcat_s(log->ptr+pos, KB-pos, "\n");
            log->len = strlen(log->ptr);
			logqueue->pushLog(log);
        }
    }
}
