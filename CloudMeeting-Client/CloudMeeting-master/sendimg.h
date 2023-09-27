#ifndef SENDIMG_H
#define SENDIMG_H

//图片发送
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QImage>
#include <QVideoFrame>              //封装了一个视频帧的像素数据,以及该帧的信息
#include <QWaitCondition>


class SendImg : public QThread      //w69
{
    Q_OBJECT
private:
    QQueue<QByteArray> imgqueue;
    QMutex queue_lock;
    QWaitCondition queue_waitCond;
    void run() override;
    QMutex m_lock;
    volatile bool m_isCanRun;
public:
    SendImg(QObject *par = NULL);

    void pushToQueue(QImage);
public slots:
    void ImageCapture(QImage); //捕获到视频帧         w105
    void clearImgQueue(); //线程结束时，清空视频帧队列   w109
    void stopImmediately();         //w211
};

#endif // SENDIMG_H
