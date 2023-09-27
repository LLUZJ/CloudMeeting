//视频显示控件

#include "myvideosurface.h"
#include <QVideoSurfaceFormat>      //捕获视频帧
#include <QDebug>
MyVideoSurface::MyVideoSurface(QObject *parent):QAbstractVideoSurface(parent)
{

}

//list容器<视频帧::像素格式> 我的视频控件::支持的像素格式 (抽象视频缓存区 :: 处理类型 handleType)
QList<QVideoFrame::PixelFormat> MyVideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    if(handleType == QAbstractVideoBuffer::NoHandle)            //如果处理类型 = 不处理
    {                                               //像素格式
        return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32                   //存储使用32位RGB格式的图像(0xffrrggbb)
                                                 << QVideoFrame::Format_ARGB32                  //存储使用32为ARGB格式的图像(0xaarrggbb)
                                                 << QVideoFrame::Format_ARGB32_Premultiplied    //图像存储使用一个自左乘32位ARGB格式
                                                 << QVideoFrame::Format_RGB565
                                                 << QVideoFrame::Format_RGB555;                 //图像存储使用16位RGB格式（5-5-5），位置用的最重要的始终为零
    }
    else
    {
        return QList<QVideoFrame::PixelFormat>();
    }
}


bool MyVideoSurface::isFormatSupported(const QVideoSurfaceFormat &format) const     //检测视频流的格式是否合法，返回bool
{
    // imageFormatFromPixelFormat: 返回与视频帧像素格式等效的图像格式
    //pixelFormat: 返回视频流中的像素格式
    return QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat()) != QImage::Format_Invalid;     //Format_Invalid：图像无效
}

//将视频流中像素格式转换成格式对等的图片格式
bool MyVideoSurface::start(const QVideoSurfaceFormat &format)
{
    if(isFormatSupported(format) && !format.frameSize().isEmpty())
    {
        QAbstractVideoSurface::start(format);
        return true;
    }
    return false;
}


//捕获每一帧视频，每一帧都会到present
bool MyVideoSurface::present(const QVideoFrame &frame)
{
    if(!frame.isValid())        //如果框架frame不是 有效的
    {
        stop();                 //停止
        return false;
    }
    if(frame.isMapped())        //如果框架frame 是映射的
    {
        emit frameAvailable(frame);     //发送信号frameAvailable(frame)，主窗口调用cameraImageCapture视频图片处理  w104
    }
    else
    {
        QVideoFrame f(frame);
        f.map(QAbstractVideoBuffer::ReadOnly);      //抽象视频缓存区 :: 只读
        emit frameAvailable(f);         //发送信号frameAvailable(f)，主窗口调用cameraImageCapture视频图片处理  w104
    }

    return true;
}
