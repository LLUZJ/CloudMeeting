//播放声音

#include "AudioOutput.h"            //播放声音
#include <QMutexLocker>             //该类中的所有函数都是线程安全的，上锁执行完毕后会自动解锁
#include "netheader.h"              //网络包模块
#include <QDebug>
#include <QHostAddress>             //ip地址类

#ifndef FRAME_LEN_125MS
#define FRAME_LEN_125MS 1900
#endif
extern QUEUE_DATA<MESG> audio_recv; //音频接收队列

AudioOutput::AudioOutput(QObject *parent)
	: QThread(parent)
{
    QAudioFormat format;                                //初始化
    format.setSampleRate(8000);                         //设置采样率
    format.setChannelCount(1);                          //设置通道数
    format.setSampleSize(16);                           //样本数据16位
    format.setCodec("audio/pcm");                       //播出格式位pcm格式
    format.setByteOrder(QAudioFormat::LittleEndian);    //默认小端模式
    format.setSampleType(QAudioFormat::UnSignedInt);    //无符号整形数

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();        //选择默认输出设备；defaultOutputDevice获取默认设备
    if (!info.isFormatSupported(format))                //通过bool isFormatSupported(format) 判断是否是支持format格式
	{
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";      //警告信息“后端不支持原始音频格式，无法播放音频”
		return;
	}
	audio = new QAudioOutput(format, this);
    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State) ));        //音频类发出stateChanged，AudioOutput类便调用handleStateChanged处理状态改变
    outputdevice = nullptr;                 //输出设备置空
}

AudioOutput::~AudioOutput()
{
	delete audio;
}

QString AudioOutput::errorString()          //获取当前错误信息
{
	if (audio->error() == QAudio::OpenError)
	{
        return QString("AudioOutput An error occurred opening the audio device").toUtf8();      //“打开音频设备时发生错误”
	}
	else if (audio->error() == QAudio::IOError)
	{
        return QString("AudioOutput An error occurred during read/write of audio device").toUtf8();     //“AudioOutput读写音频设备时发生错误”
	}
	else if (audio->error() == QAudio::UnderrunError)
	{
        return QString("AudioOutput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();       //“音频数据没有以足够快的速度馈送到音频设备”
	}
	else if (audio->error() == QAudio::FatalError)
	{
        return QString("AudioOutput A non-recoverable error has occurred, the audio device is not usable at this time.");       //“AudioOutput发生了不可恢复的错误，音频设备此时无法使用”
	}
	else
	{
        return QString("AudioOutput No errors have occurred").toUtf8();     //“AudioOutput没有错误发生”
	}
}

void AudioOutput::handleStateChanged(QAudio::State state)       //处理状态改变    32
{
	switch (state)
	{
        case QAudio::ActiveState:                       //活动状态；  音频播放，正在解析中
			break;
        case QAudio::SuspendedState:                    //暂停状态；  音频被暂停
			break;
        case QAudio::StoppedState:                      //停止状态；
            if (audio->error() != QAudio::NoError)          //音频设备已关闭，检查error是否异常
			{
				audio->stop();
                emit audiooutputerror(errorString());       //发出audiooutputerror信号，主窗口调用audioError函数，弹出消息框提示音频错误  w134
				qDebug() << "out audio error" << audio->error();
			}
			break;
        case QAudio::IdleState:                         //空闲状态
			break;
        case QAudio::InterruptedState:                  //中断状态
			break;
		default:
			break;
	}
}

void AudioOutput::startPlay()       //开始播放  w667
{
    if (audio->state() == QAudio::ActiveState) return;      //如果已经处于活动状态 就退出
    WRITE_LOG("start playing audio");                       //日志“开始播放音频”
    outputdevice = audio->start();                          //输出设备启动音频
}

void AudioOutput::stopPlay()        //停止播放  w689,717；   120
{
    if (audio->state() == QAudio::StoppedState) return;     //如果已经处于停止状态 就退出
	{
        QMutexLocker lock(&device_lock);                    //创建一个设备锁
		outputdevice = nullptr;
	}
    audio->stop();                                          //停止音频
    WRITE_LOG("stop playing audio");                        //日志“停止播放音频”
}

void AudioOutput::run()             //AudioOutput继承与QThread，重写run方法
{
	is_canRun = true;
    QByteArray m_pcmDataBuffer;     //pcm数据缓存

    WRITE_LOG("start playing audio thread 0x%p", QThread::currentThreadId());       //日志“开始播放音乐线程”，currentThreadId获取线程id
	for (;;)
	{
		{
			QMutexLocker lock(&m_lock);
            if (is_canRun == false)         //is_canRun可以运行，置为false
			{
                stopPlay();                 //调用停止播放
                WRITE_LOG("stop playing audio thread 0x%p", QThread::currentThreadId());        //日志“停止播放音乐线程”
				return;
			}
		}
        MESG* msg = audio_recv.pop_msg();       //音频接收.pop_msg
        if (msg == NULL) continue;              //如果msg为空，直接跳出循环
		{
			QMutexLocker lock(&device_lock);
            if (outputdevice != nullptr)        //如果输出设备不为空
			{
                m_pcmDataBuffer.append((char*)msg->data, msg->len);         //往pcm数据缓存后追加（data,len）

                if (m_pcmDataBuffer.size() >= FRAME_LEN_125MS)              //如果pcm数据缓存的大小>=FRAME_LEN_125MS
				{
					//写入音频数据
                    qint64 ret = outputdevice->write(m_pcmDataBuffer.data(), FRAME_LEN_125MS);      //输出设备 写入pcm数据缓存数据，数据大小为FRAME_LEN_125MS 存入ret中
                    if (ret < 0)
					{
                        qDebug() << outputdevice->errorString();            //打印输出设备错误信息
						return;
					}
					else
					{
                        emit speaker(QHostAddress(msg->ip).toString());     //发出speaker信号，主窗口调用speaks函数，outlog输出日志打印xxip正在说话  w135
                        m_pcmDataBuffer = m_pcmDataBuffer.right(m_pcmDataBuffer.size() - ret);      //对pcm数据缓存 从右往左截取（pcm数据缓存大小-ret）个字符 ;right:从右向左截取n个字符串
					}
				}
			}
			else
			{
                m_pcmDataBuffer.clear();                                    //清空pcm数据缓存
			}
		}
        if (msg->data) free(msg->data);                                     //如果msg的data存在，则释放掉
		if (msg) free(msg);
	}
}
void AudioOutput::stopImmediately()     //立刻停止  w235
{
	QMutexLocker lock(&m_lock);
    is_canRun = false;                  //is_canRun可以运行 置 false     -->118
}


void AudioOutput::setVolumn(int val)        //设置音量
{
	audio->setVolume(val / 100.0);
}

void AudioOutput::clearQueue()
{
    qDebug() << "audio recv clear";         //打印“音频接收清除”
    audio_recv.clear();                     //清除掉音频接收
}
