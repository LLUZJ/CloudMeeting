#ifndef NETHEADER_H
#define NETHEADER_H

//网络包模块
#include <QMetaType>            //管理元对象系统中的命名类型
#include <QMutex>               //互斥锁
#include <QQueue>               //是一个C++实现的队列类，提供了丰富的API以实现队列的各种操作
#include <QImage>               //提供独立于硬件的图像表示，允许直接访问像素数据，并可用作绘制设备
#include <QWaitCondition>       //用于多线程的同步，一个线程调用QWaitCondition::wait() 阻塞等待，直到另一个线程调用QWaitCondition::wake() 唤醒才继续往下执行。
#define QUEUE_MAXSIZE 1500          //si92;     st24
#ifndef MB
#define MB 1024*1024
#endif

#ifndef KB
#define KB 1024
#endif

#ifndef WAITSECONDS
#define WAITSECONDS 2               //si28;     st44
#endif

#ifndef OPENVIDEO
#define OPENVIDEO "打开视频"        //w728
#endif

#ifndef CLOSEVIDEO
#define CLOSEVIDEO "关闭视频"
#endif

#ifndef OPENAUDIO
#define OPENAUDIO "打开音频"        //w352,360,690
#endif

#ifndef CLOSEAUDIO
#define CLOSEAUDIO "关闭音频"       //w355,357,718
#endif


#ifndef MSG_HEADER
#define MSG_HEADER 11               //消息头   mts224，246
#endif

enum MSG_TYPE       //w39,93;   mts236;     st20
{
    IMG_SEND = 0,                       //发送图片  mts95;    si58
    IMG_RECV,                           //接收图片  w511
    AUDIO_SEND,                         //发送音频  AI82;     mts95
    AUDIO_RECV,                         //接收音频
    TEXT_SEND,                          //发送信息  w801;     mts95，137
    TEXT_RECV,                          //接收信息  w535
    CREATE_MEETING,                     //创建会议  w258;     mts95;    st77
    EXIT_MEETING,                       //退出会议
    JOIN_MEETING,                       //加入会议  mts102
    CLOSE_CAMERA,                       //关闭摄像头  w332,578;     mts95;   st77

    CREATE_MEETING_RESPONSE = 20,       //创建会议响应  w436;     mts241，243
    PARTNER_EXIT = 21,                  //伙伴退出  w560
    PARTNER_JOIN = 22,                  //伙伴加入  w549
    JOIN_MEETING_RESPONSE = 23,         //加入会议响应  w469;     mts241，274
    PARTNER_JOIN2 = 24,                 //伙伴2加入  w582;      mts241
    RemoteHostClosedError = 40,         //远程主机关闭错误  w600;   mts61
    OtherNetError = 41                  //其他网络错误      w623;   mts65
};
Q_DECLARE_METATYPE(MSG_TYPE);

struct MESG //消息结构体     //w114;     AI73;   AO125;  mts51,174，279;   rs17,34;   si49;   st66
{
    MSG_TYPE msg_type;      //w436,469,511,535,549,560,578,582,600,623  ;   AI82;   mts61,65;   si58;   st81
    uchar* data;            //w439,472,516,537,588,631
    long len;               //w439,472,516,537,585;                         si58
    quint32 ip;             //w513,543,551,562
};
Q_DECLARE_METATYPE(MESG *);

//-------------------------------

template<class T>
struct QUEUE_DATA //消息队列
{
private:
    QMutex send_queueLock;
    QWaitCondition send_queueCond;
    QQueue<T*> send_queue;
public:
    void push_msg(T* msg)           //AI97;  lq13;  si74;   st82
    {
        send_queueLock.lock();
        while(send_queue.size() > QUEUE_MAXSIZE)
        {
            send_queueCond.wait(&send_queueLock);
        }
        send_queue.push_back(msg);
        send_queueLock.unlock();

        send_queueCond.wakeOne();
    }

    T* pop_msg()                    //AO125;    lq29;   mts174;     rs34
    {
        send_queueLock.lock();
        while(send_queue.size() == 0)
        {
            bool f = send_queueCond.wait(&send_queueLock, WAITSECONDS * 1000);
            if(f == false)
            {
                send_queueLock.unlock();
                return NULL;
            }
        }
        T* send = send_queue.front();
        send_queue.pop_front();
        send_queueLock.unlock();
        send_queueCond.wakeOne();
        return send;
    }

    void clear()
    {
        send_queueLock.lock();
        send_queue.clear();
        send_queueLock.unlock();
    }
};

struct Log          //lq29;     16
{
    char *ptr;
    uint len;
};



void log_print(const char *, const char *, int, const char *, ...);
//w41,238,259,314,326,339,410,418,452,465,478,490,497,574   ；Al46，56；   AO93，105，113，121;   rs23,30;    si19，35，52，65;      st38，51，69，92，115
#define WRITE_LOG(LOGTEXT, ...) do{ \
    log_print(__FILE__, __FUNCTION__, __LINE__, LOGTEXT, ##__VA_ARGS__);\
}while(0);



#endif // NETHEADER_H
