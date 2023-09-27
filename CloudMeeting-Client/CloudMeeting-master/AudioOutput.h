#pragma once

//播放声音
#include <QObject>
#include <QThread>
#include <QAudioOutput>
#include <QMutex>               //互斥锁
class AudioOutput : public QThread      //w127
{
	Q_OBJECT
private:
	QAudioOutput* audio;
	QIODevice* outputdevice;
	QMutex device_lock;
	
	volatile bool is_canRun;
	QMutex m_lock;
	void run();
	QString errorString();
public:
	AudioOutput(QObject *parent = 0);
	~AudioOutput();
    void stopImmediately();                     //w235
    void startPlay();                           //w667
    void stopPlay();                            //w689,717
private slots:
	void handleStateChanged(QAudio::State);
	void setVolumn(int);
	void clearQueue();
signals:
    void audiooutputerror(QString);             //w134
    void speaker(QString);                      //w135
};
