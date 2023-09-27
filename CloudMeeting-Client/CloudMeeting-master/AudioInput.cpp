//¼��

#include "AudioInput.h"             //¼��
#include "netheader.h"              //�����ģ��
#include <QAudioFormat>             //�����������Ƶ���ݵĽӿ�
#include <QDebug>
#include <QThread>

extern QUEUE_DATA<MESG> queue_send;
extern QUEUE_DATA<MESG> queue_recv;


AudioInput::AudioInput(QObject *parent)
	: QObject(parent)
{
    recvbuf = (char*)malloc(MB * 2);    //mallocʱ��̬�ڴ���亯������������һ��������ָ����С���ڴ��������void*���ͷ��ط�����ڴ������ַ
    //����pcm���ݲ�����ʽ
    QAudioFormat format;
	//set format
    format.setSampleRate(8000);                             //���ò�����
    format.setChannelCount(1);                              //����ͨ������mono(ƽ����)��������Ŀ��1��stero(������)��������Ŀ��2��
    format.setSampleSize(16);                               //���ò�����С
    format.setCodec("audio/pcm");                           //���ñ��뷽ʽ����������
    format.setByteOrder(QAudioFormat::LittleEndian);        //�����ֽ���(�ߵ�λ)�������ǵ�λ����
    format.setSampleType(QAudioFormat::UnSignedInt);        //����������������

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

void AudioInput::startCollect()         //��ʼ�ռ���Ƶ w131
{
    if (audio->state() == QAudio::ActiveState) return;          //���audio��״̬�Ѿ��ǿ���״̬���˳�
    WRITE_LOG("start collecting audio");                        //��־����ʼ�ռ���Ƶ��
    inputdevice = audio->start();                               //����audio,inputdevice�����ʽ��io�豸
    connect(inputdevice, SIGNAL(readyRead()), this, SLOT(onreadyRead()));   //��inputdevice����readyRead�źţ�AudioInput�����onreadyRead
}

void AudioInput::stopCollect()          //ֹͣ�ռ���Ƶ w132,688,716
{
    if (audio->state() == QAudio::StoppedState) return;         //���audio��״̬�Ѿ���ֹͣ״̬���˳�
    disconnect(this, SLOT(onreadyRead()));                      //�Ͽ���48��ֹͣ׼����ȡ��Ƶ
    audio->stop();                                              //ֹͣaduio
    WRITE_LOG("stop collecting audio");                         //��־��ֹͣ�ռ���Ƶ��
    inputdevice = nullptr;                                      //inputdevice�ÿ�
}

void AudioInput::onreadyRead()          //׼����ȡ  48
{
	static int num = 0, totallen  = 0;
    if (inputdevice == nullptr) return;     //���inputdevice�ÿվ��˳�
    int len = inputdevice->read(recvbuf + totallen, 2 * MB - totallen);     //recvbuf = (char*)malloc(MB * 2) ��16�ж�����
	if (num < 2)
	{
		totallen += len;
		num++;
		return;
    }
    totallen += len;
	qDebug() << "totallen = " << totallen;

    MESG* msg = (MESG*)malloc(sizeof(MESG));            //malloc��̬�ڴ���亯��
	if (msg == nullptr)
	{
        qWarning() << __LINE__ << "malloc fail";        //���������Ϣ���ռ�ʧ�ܡ�
	}
	else
	{
        memset(msg, 0, sizeof(MESG));                   //memset���ֽ�Ϊ��λ��ֵ����0��ʾ��00000000��ͨ�����Ƶķ�ʽ��չ��sizeof(MESG)�ֽڣ�Ȼ����sizeof(MESG)�ֽڸ����������ͷָ��msgָ��ĵ�ַ��Ԫ��ʼ��ֵ��ֱ����ȫ���ֽ�����
		msg->msg_type = AUDIO_SEND;
		//ѹ�����ݣ�תbase64
		QByteArray rr(recvbuf, totallen);
        QByteArray cc = qCompress(rr).toBase64();       //������תΪbase64��ʽ
		msg->len = cc.size();

		msg->data = (uchar*)malloc(msg->len);
		if (msg->data == nullptr)
		{
            qWarning() << "malloc mesg.data fail";      //���������Ϣ���ռ���Ϣ����ʧ�ܡ�
		}
		else
		{
            memset(msg->data, 0, msg->len);             //memset���ֽ�Ϊ��λ��ֵ�����ַ�0ͨ�����Ƶķ�ʽ��չ��msg->len�ֽڣ�Ȼ����msg->len�ֽڸ����������ͷָ��msg->dataָ��ĵ�ַ��Ԫ��ʼ��ֵ��ֱ����ȫ���ֽ�����
			memcpy_s(msg->data, msg->len, cc.data(), cc.size());
            queue_send.push_msg(msg);                   //���з��ͣ�����push_msg����msg�ӵ������������
		}
	}
	totallen = 0;
	num = 0;
}

QString AudioInput::errorString()                       //�����ַ�
{
    if (audio->error() == QAudio::OpenError)
	{
        return QString("AudioInput An error occurred opening the audio device").toUtf8();       //������Ƶ�豸ʱ��������
	}
    else if (audio->error() == QAudio::IOError)
	{
        return QString("AudioInput An error occurred during read/write of audio device").toUtf8();      //����д��Ƶ�豸ʱ��������
	}
	else if (audio->error() == QAudio::UnderrunError)
	{
        return QString("AudioInput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();        //����Ƶ����û�����㹻����ٶ����͵���Ƶ�豸��
	}
	else if (audio->error() == QAudio::FatalError)
	{
        return QString("AudioInput A non-recoverable error has occurred, the audio device is not usable at this time.");        //�������˲��ɻָ��Ĵ�����Ƶ�豸��ʱ�޷�ʹ�á�
	}
	else
	{
        return QString("AudioInput No errors have occurred").toUtf8();      //��û�д�������
	}
}

void AudioInput::handleStateChanged(QAudio::State newState)                 //����״̬�ı�
{
	switch (newState)
	{
        case QAudio::StoppedState:                        //ֹͣ״̬
            if (audio->error() != QAudio::NoError)          //������ǡ��޴���
			{
                stopCollect();                              //����ֹͣ�ռ� 51
                emit audioinputerror(errorString());        //����audioinputerror�źţ������ڵ���audioError������������Ϣ����ʾ��Ƶ����    w133
			}
			else
			{
                qWarning() << "stop recording";             //��ӡ������Ϣ��ֹͣ¼����
			}
			break;
        case QAudio::ActiveState:                         //�״̬
			//start recording
            qWarning() << "start recording";                //������Ϣ����ʼ¼����
			break;
		default:
			//
			break;
	}
}

void AudioInput::setVolumn(int v)       //��������  w662,663,686,687,713,714
{
	qDebug() << v;
	audio->setVolume(v / 100.0);
}
