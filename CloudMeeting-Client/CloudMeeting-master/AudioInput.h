#pragma once

//录音
#include <QObject>
#include <QAudioInput>
#include <QIODevice>            //Qt中所有I/O设备的抽象基类，为支持读写数据块的设备提供了通用实现和其他接口
class AudioInput : public QObject           //w122
{
	Q_OBJECT
private:
	QAudioInput *audio;
	QIODevice* inputdevice;
	char* recvbuf;
public:
	AudioInput(QObject *par = 0);
	~AudioInput();
private slots:
	void onreadyRead();
	void handleStateChanged(QAudio::State);
	QString errorString();
	void setVolumn(int);
public slots:
    void startCollect();                //w131
    void stopCollect();                 //w132,688,716
signals:
    void audioinputerror(QString);      //w133
};
