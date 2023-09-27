//录音

#include "AudioInput.h"             //录音
#include "netheader.h"              //网络包模块
#include <QAudioFormat>             //从外设接收音频数据的接口
#include <QDebug>
#include <QThread>

extern QUEUE_DATA<MESG> queue_send;
extern QUEUE_DATA<MESG> queue_recv;


AudioInput::AudioInput(QObject *parent)
	: QObject(parent)
{
    recvbuf = (char*)malloc(MB * 2);    //malloc时动态内存分配函数，用于申请一块连续的指定大小的内存块区域以void*类型返回分配的内存区域地址
    //设置pcm数据采样格式
    QAudioFormat format;
	//set format
    format.setSampleRate(8000);                             //设置采样率
    format.setChannelCount(1);                              //设置通道数（mono(平声道)的声道数目是1；stero(立体声)的声道数目是2）
    format.setSampleSize(16);                               //设置采样大小
    format.setCodec("audio/pcm");                           //设置编码方式（编码器）
    format.setByteOrder(QAudioFormat::LittleEndian);        //设置字节序(高低位)，这里是低位优先
    format.setSampleType(QAudioFormat::UnSignedInt);        //设置样本数据类型

	QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
	if (!info.isFormatSupported(format))
	{
		qWarning() << "Default format not supported, trying to use the nearest.";
		format = info.nearestFormat(format);
	}
	audio = new QAudioInput(format, this);
	connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));

}

AudioInput::~AudioInput()
{
	delete audio;
}

void AudioInput::startCollect()         //开始收集音频 w131
{
    if (audio->state() == QAudio::ActiveState) return;          //如果audio的状态已经是开启状态就退出
    WRITE_LOG("start collecting audio");                        //日志“开始收集音频”
    inputdevice = audio->start();                               //启动audio,inputdevice输出方式是io设备
    connect(inputdevice, SIGNAL(readyRead()), this, SLOT(onreadyRead()));   //当inputdevice发出readyRead信号，AudioInput类调用onreadyRead
}

void AudioInput::stopCollect()          //停止收集音频 w132,688,716
{
    if (audio->state() == QAudio::StoppedState) return;         //如果audio的状态已经是停止状态就退出
    disconnect(this, SLOT(onreadyRead()));                      //断开槽48，停止准备读取音频
    audio->stop();                                              //停止aduio
    WRITE_LOG("stop collecting audio");                         //日志“停止收集音频”
    inputdevice = nullptr;                                      //inputdevice置空
}

void AudioInput::onreadyRead()          //准备读取  48
{
	static int num = 0, totallen  = 0;
    if (inputdevice == nullptr) return;     //如果inputdevice置空就退出
    int len = inputdevice->read(recvbuf + totallen, 2 * MB - totallen);     //recvbuf = (char*)malloc(MB * 2) 在16中定义了
	if (num < 2)
	{
		totallen += len;
		num++;
		return;
    }
    totallen += len;
	qDebug() << "totallen = " << totallen;

    MESG* msg = (MESG*)malloc(sizeof(MESG));            //malloc动态内存分配函数
	if (msg == nullptr)
	{
        qWarning() << __LINE__ << "malloc fail";        //输出警告信息“收集失败”
	}
	else
	{
        memset(msg, 0, sizeof(MESG));                   //memset以字节为单位赋值，将0表示的00000000，通过复制的方式扩展至sizeof(MESG)字节，然后将这sizeof(MESG)字节根据数据类型分割并从msg指向的地址单元开始赋值，直到将全部字节用完
		msg->msg_type = AUDIO_SEND;
		//压缩数据，转base64
		QByteArray rr(recvbuf, totallen);
        QByteArray cc = qCompress(rr).toBase64();       //将数据转为base64格式
		msg->len = cc.size();

		msg->data = (uchar*)malloc(msg->len);
		if (msg->data == nullptr)
		{
            qWarning() << "malloc mesg.data fail";      //输出警告信息“收集信息数据失败”
		}
		else
		{
            memset(msg->data, 0, msg->len);             //memset以字节为单位赋值，将字符0通过复制的方式扩展至msg->len字节，然后将这msg->len字节根据数据类型分割并从msg->data指向的地址单元开始赋值，直到将全部字节用完
			memcpy_s(msg->data, msg->len, cc.data(), cc.size());
            queue_send.push_msg(msg);                   //队列发送，调用push_msg，把msg加到容器的最后面
		}
	}
	totallen = 0;
	num = 0;
}

QString AudioInput::errorString()                       //错误字符
{
    if (audio->error() == QAudio::OpenError)
	{
        return QString("AudioInput An error occurred opening the audio device").toUtf8();       //“打开音频设备时发生错误”
	}
    else if (audio->error() == QAudio::IOError)
	{
        return QString("AudioInput An error occurred during read/write of audio device").toUtf8();      //“读写音频设备时发生错误”
	}
	else if (audio->error() == QAudio::UnderrunError)
	{
        return QString("AudioInput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();        //“音频数据没有以足够快的速度馈送到音频设备”
	}
	else if (audio->error() == QAudio::FatalError)
	{
        return QString("AudioInput A non-recoverable error has occurred, the audio device is not usable at this time.");        //“发生了不可恢复的错误，音频设备此时无法使用”
	}
	else
	{
        return QString("AudioInput No errors have occurred").toUtf8();      //“没有错误发生”
	}
}

void AudioInput::handleStateChanged(QAudio::State newState)                 //处理状态改变
{
	switch (newState)
	{
        case QAudio::StoppedState:                        //停止状态
            if (audio->error() != QAudio::NoError)          //如果不是“无错误”
			{
                stopCollect();                              //调用停止收集 51
                emit audioinputerror(errorString());        //发出audioinputerror信号，主窗口调用audioError函数，弹出消息框提示音频错误    w133
			}
			else
			{
                qWarning() << "stop recording";             //打印警告信息“停止录音”
			}
			break;
        case QAudio::ActiveState:                         //活动状态
			//start recording
            qWarning() << "start recording";                //警告信息“开始录音”
			break;
		default:
			//
			break;
	}
}

void AudioInput::setVolumn(int v)       //设置音量  w662,663,686,687,713,714
{
	qDebug() << v;
	audio->setVolume(v / 100.0);
}
