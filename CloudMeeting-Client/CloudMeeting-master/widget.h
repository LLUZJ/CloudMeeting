#ifndef WIDGET_H
#define WIDGET_H

//主窗口
#include "AudioInput.h"             //录音
#include <QWidget>
#include <QVideoFrame>                  //视频帧的像素数据
#include <QTcpSocket>                   //Tcp套接字
#include "mytcpsocket.h"            //网络通信
#include <QCamera>
#include "sendtext.h"               //文字发送
#include "recvsolve.h"              //数据接收模块
#include "partner.h"                //房间信息
#include "netheader.h"              //网络包模块
#include <QMap>
#include "AudioOutput.h"            //播放声音
#include "chatmessage.h"            //聊天信息
#include <QStringListModel>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class QCameraImageCapture;
class MyVideoSurface;
class SendImg;
class QListWidgetItem;
class Widget : public QWidget
{
    Q_OBJECT
private:
    static QRect pos;
    quint32 mainip;         //主屏幕显示的IP图像        w65,173,374,455,501,528,563,734
    QCamera *_camera;       //摄像头               w97,99,100,118,119,278,323
    QCameraImageCapture *_imagecapture; //截屏    w100
    bool  _createmeet;      //是否创建会议         w44,252,351,447,461,621
    bool _joinmeet;         // 加入会议           w46,351,483,507,621
    bool _openCamera;       //是否开启摄像头       w45

    MyVideoSurface *_myvideosurface;        //w101,104,118

    QVideoFrame mainshow;

    SendImg *_sendImg;              //w69,71,105,109,209,
    QThread *_imgThread;            //w70,71,109,203,342,723

    RecvSolve *_recvThread;         //w113,114,115,197
    SendText * _sendText;           //w87,89,91,93,221
    QThread *_textThread;           //w88,90,215,

    //socket
    MyTcpSocket * _mytcpSocket;     //w80,81,173,178,190,293,334,402,454,500,544,604,627
    void paintEvent(QPaintEvent *event);        //w263

    QMap<quint32, Partner *> partner; //用于记录房间用户    w178,366,517,645,675,699,701,734
    Partner* addPartner(quint32);       //w454,500,551,590,643
    void removePartner(quint32);        //w562,673
    void clearPartner(); //退出会议，或者会议结束      w289,603,626,696
    void closeImg(quint32); //根据IP重置图像          w334,364,580

    void dealMessage(ChatMessage *messageW, QListWidgetItem *item, QString text, QString time, QString ip ,ChatMessage::User_Type type); //用户发送文本       543,800,805
    void dealMessageTime(QString curMsgTime); //处理时间        w542,799,815

    //音频
    AudioInput* _ainput;        //w122,124,131,132,133,662,686,688,713,716
    QThread* _ainputThread;     //w123,124,128,227
    AudioOutput* _aoutput;      //w127,129,134,135,233,663,667,687,689,714,717

    QStringList iplist;         //w310,556,567,594,619

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_createmeetBtn_clicked();            //w250
    void on_exitmeetBtn_clicked();              //w276

    void on_openVedio_clicked();                //w321
    void on_openAudio_clicked();                //w349
    void on_connServer_clicked();               //w380
    void cameraError(QCamera::Error);           //w99,424
    void audioError(QString);                   //w133,134,429
//    void mytcperror(QAbstractSocket::SocketError);
    void datasolve(MESG *);                     //w114,434
    void recvip(quint32);                       //w654,678,732
    void cameraImageCapture(QVideoFrame frame);     //w155
    void on_joinmeetBtn_clicked();              //w755

    void on_horizontalSlider_valueChanged(int value);   //w774
    void speaks(QString);                       //w135,779

    void on_sendmsg_clicked();                  //w784

    void textSend();                            //w81,842

signals:
    void pushImg(QImage);                           //w105
    void PushText(MSG_TYPE, QString = "");          //w93,258,332,770,801
    void stopAudio();                               //w132,359
    void startAudio();                              //w131,354
    void volumnChange(int);                         //w662,663,776
private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
