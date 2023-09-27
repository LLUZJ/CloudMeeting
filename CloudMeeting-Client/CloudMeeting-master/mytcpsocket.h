#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

//网络通信
#include <QThread>
#include <QTcpSocket>
#include <QMutex>               //互斥锁
#include "netheader.h"          //网络包模块
#ifndef MB
#define MB (1024 * 1024)
#endif

typedef unsigned char uchar;


class MyTcpSocket: public QThread       //w80
{
    Q_OBJECT
public:
    ~MyTcpSocket();
    MyTcpSocket(QObject *par=NULL);
    bool connectToServer(QString, QString, QIODevice::OpenModeFlag);        //w402
    QString errorString();
    void disconnectFromHost();          //w293,604,627
    quint32 getlocalip();                //w173,178,334,454,500,544
private:
    void run() override;
    qint64 readn(char *, quint64, int);
    QTcpSocket *_socktcp;
    QThread *_sockThread;
    uchar *sendbuf;
    uchar* recvbuf;
    quint64 hasrecvive;

    QMutex m_lock;
    volatile bool m_isCanRun;
private slots:
    bool connectServer(QString, QString, QIODevice::OpenModeFlag);
    void sendData(MESG *);
    void closeSocket();

public slots:
    void recvFromSocket();
    void stopImmediately();         //w192
    void errorDetect(QAbstractSocket::SocketError error);
signals:
    void socketerror(QAbstractSocket::SocketError);
    void sendTextOver();            //w81;  77,139
};

#endif // MYTCPSOCKET_H
