//��������

#include "AudioOutput.h"            //��������
#include <QMutexLocker>             //�����е����к��������̰߳�ȫ�ģ�����ִ����Ϻ���Զ�����
#include "netheader.h"              //�����ģ��
#include <QDebug>
#include <QHostAddress>             //ip��ַ��

#ifndef FRAME_LEN_125MS
#define FRAME_LEN_125MS 1900
#endif
extern QUEUE_DATA<MESG> audio_recv; //��Ƶ���ն���

AudioOutput::AudioOutput(QObject *parent)
	: QThread(parent)
{
    QAudioFormat format;                                //��ʼ��
    format.setSampleRate(8000);                         //���ò�����
    format.setChannelCount(1);                          //����ͨ����
    format.setSampleSize(16);                           //��������16λ
    format.setCodec("audio/pcm");                       //������ʽλpcm��ʽ
    format.setByteOrder(QAudioFormat::LittleEndian);    //Ĭ��С��ģʽ
    format.setSampleType(QAudioFormat::UnSignedInt);    //�޷���������

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();        //ѡ��Ĭ������豸��defaultOutputDevice��ȡĬ���豸
    if (!info.isFormatSupported(format))                //ͨ��bool isFormatSupported(format) �ж��Ƿ���֧��format��ʽ
	{
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";      //������Ϣ����˲�֧��ԭʼ��Ƶ��ʽ���޷�������Ƶ��
		return;
	}
	audio = new QAudioOutput(format, this);
    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State) ));        //��Ƶ�෢��stateChanged��AudioOutput������handleStateChanged����״̬�ı�
    outputdevice = nullptr;                 //����豸�ÿ�
}

AudioOutput::~AudioOutput()
{
	delete audio;
}

QString AudioOutput::errorString()          //��ȡ��ǰ������Ϣ
{
	if (audio->error() == QAudio::OpenError)
	{
        return QString("AudioOutput An error occurred opening the audio device").toUtf8();      //������Ƶ�豸ʱ��������
	}
	else if (audio->error() == QAudio::IOError)
	{
        return QString("AudioOutput An error occurred during read/write of audio device").toUtf8();     //��AudioOutput��д��Ƶ�豸ʱ��������
	}
	else if (audio->error() == QAudio::UnderrunError)
	{
        return QString("AudioOutput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();       //����Ƶ����û�����㹻����ٶ����͵���Ƶ�豸��
	}
	else if (audio->error() == QAudio::FatalError)
	{
        return QString("AudioOutput A non-recoverable error has occurred, the audio device is not usable at this time.");       //��AudioOutput�����˲��ɻָ��Ĵ�����Ƶ�豸��ʱ�޷�ʹ�á�
	}
	else
	{
        return QString("AudioOutput No errors have occurred").toUtf8();     //��AudioOutputû�д�������
	}
}

void AudioOutput::handleStateChanged(QAudio::State state)       //����״̬�ı�    32
{
	switch (state)
	{
        case QAudio::ActiveState:                       //�״̬��  ��Ƶ���ţ����ڽ�����
			break;
        case QAudio::SuspendedState:                    //��ͣ״̬��  ��Ƶ����ͣ
			break;
        case QAudio::StoppedState:                      //ֹͣ״̬��
            if (audio->error() != QAudio::NoError)          //��Ƶ�豸�ѹرգ����error�Ƿ��쳣
			{
				audio->stop();
                emit audiooutputerror(errorString());       //����audiooutputerror�źţ������ڵ���audioError������������Ϣ����ʾ��Ƶ����  w134
				qDebug() << "out audio error" << audio->error();
			}
			break;
        case QAudio::IdleState:                         //����״̬
			break;
        case QAudio::InterruptedState:                  //�ж�״̬
			break;
		default:
			break;
	}
}

void AudioOutput::startPlay()       //��ʼ����  w667
{
    if (audio->state() == QAudio::ActiveState) return;      //����Ѿ����ڻ״̬ ���˳�
    WRITE_LOG("start playing audio");                       //��־����ʼ������Ƶ��
    outputdevice = audio->start();                          //����豸������Ƶ
}

void AudioOutput::stopPlay()        //ֹͣ����  w689,717��   120
{
    if (audio->state() == QAudio::StoppedState) return;     //����Ѿ�����ֹͣ״̬ ���˳�
	{
        QMutexLocker lock(&device_lock);                    //����һ���豸��
		outputdevice = nullptr;
	}
    audio->stop();                                          //ֹͣ��Ƶ
    WRITE_LOG("stop playing audio");                        //��־��ֹͣ������Ƶ��
}

void AudioOutput::run()             //AudioOutput�̳���QThread����дrun����
{
	is_canRun = true;
    QByteArray m_pcmDataBuffer;     //pcm���ݻ���

    WRITE_LOG("start playing audio thread 0x%p", QThread::currentThreadId());       //��־����ʼ���������̡߳���currentThreadId��ȡ�߳�id
	for (;;)
	{
		{
			QMutexLocker lock(&m_lock);
            if (is_canRun == false)         //is_canRun�������У���Ϊfalse
			{
                stopPlay();                 //����ֹͣ����
                WRITE_LOG("stop playing audio thread 0x%p", QThread::currentThreadId());        //��־��ֹͣ���������̡߳�
				return;
			}
		}
        MESG* msg = audio_recv.pop_msg();       //��Ƶ����.pop_msg
        if (msg == NULL) continue;              //���msgΪ�գ�ֱ������ѭ��
		{
			QMutexLocker lock(&device_lock);
            if (outputdevice != nullptr)        //�������豸��Ϊ��
			{
                m_pcmDataBuffer.append((char*)msg->data, msg->len);         //��pcm���ݻ����׷�ӣ�data,len��

                if (m_pcmDataBuffer.size() >= FRAME_LEN_125MS)              //���pcm���ݻ���Ĵ�С>=FRAME_LEN_125MS
				{
					//д����Ƶ����
                    qint64 ret = outputdevice->write(m_pcmDataBuffer.data(), FRAME_LEN_125MS);      //����豸 д��pcm���ݻ������ݣ����ݴ�СΪFRAME_LEN_125MS ����ret��
                    if (ret < 0)
					{
                        qDebug() << outputdevice->errorString();            //��ӡ����豸������Ϣ
						return;
					}
					else
					{
                        emit speaker(QHostAddress(msg->ip).toString());     //����speaker�źţ������ڵ���speaks������outlog�����־��ӡxxip����˵��  w135
                        m_pcmDataBuffer = m_pcmDataBuffer.right(m_pcmDataBuffer.size() - ret);      //��pcm���ݻ��� ���������ȡ��pcm���ݻ����С-ret�����ַ� ;right:���������ȡn���ַ���
					}
				}
			}
			else
			{
                m_pcmDataBuffer.clear();                                    //���pcm���ݻ���
			}
		}
        if (msg->data) free(msg->data);                                     //���msg��data���ڣ����ͷŵ�
		if (msg) free(msg);
	}
}
void AudioOutput::stopImmediately()     //����ֹͣ  w235
{
	QMutexLocker lock(&m_lock);
    is_canRun = false;                  //is_canRun�������� �� false     -->118
}


void AudioOutput::setVolumn(int val)        //��������
{
	audio->setVolume(val / 100.0);
}

void AudioOutput::clearQueue()
{
    qDebug() << "audio recv clear";         //��ӡ����Ƶ���������
    audio_recv.clear();                     //�������Ƶ����
}
