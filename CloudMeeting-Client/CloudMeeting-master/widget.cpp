//主窗口

#include "widget.h"
#include "ui_widget.h"
#include "screen.h"                 //屏幕工具模块
#include <QCamera>                      //通过视点渲染场景
#include <QCameraViewfinder>            //相机取景器部件
#include <QCameraImageCapture>          //录制媒体内容
#include <QDebug>
#include <QPainter>
#include "myvideosurface.h"         //视频显示控件
#include "sendimg.h"                //图片发送
#include <QRegExp>                      //提供模式匹配
#include <QRegExpValidator>             //根据正则表达式检查字符串
#include <QMessageBox>
#include <QScrollBar>                   //垂直或水平滚动条
#include <QHostAddress>
#include <QTextCodec>                   //文本编码转换
#include "logqueue.h"               //日志队列
#include <QDateTime>                    //日期和时间函数
#include <QCompleter>                   //基于项模型的补全功能
#include <QStringListModel>             //向视图提供字符串的模型
#include <QSound>                       //播放wav音乐文件的方法
QRect  Widget::pos = QRect(-1, -1, -1, -1);

extern LogQueue *logqueue;              //此变量在别处定义，在此引用

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{


    //开启日志线程
    logqueue = new LogQueue();
    logqueue->start();

    qDebug() << "main: " <<QThread::currentThread();                //打印线程号
    qRegisterMetaType<MSG_TYPE>();                                  //注册自定义类型到元对象系统

    WRITE_LOG("-------------------------Application Start---------------------------");
    WRITE_LOG("main UI thread id: 0x%p", QThread::currentThreadId());
    //ui界面
    _createmeet = false;            //是否创建会议，置否
    _openCamera = false;            //是否开启摄像头，置否
    _joinmeet = false;              //是否加入会议，置否
    Widget::pos = QRect(0.1 * Screen::width, 0.1 * Screen::height, 0.8 * Screen::width, 0.8 * Screen::height);          //定义一个矩形

    ui->setupUi(this);

    ui->openAudio->setText(QString(OPENAUDIO).toUtf8());            //开启音频
    ui->openVedio->setText(QString(OPENVIDEO).toUtf8());            //开启摄像头

    this->setGeometry(Widget::pos);                                 //返回相对这个widget(重载了QMouseEvent的widget)的位置
    this->setMinimumSize(QSize(Widget::pos.width() * 0.7, Widget::pos.height() * 0.7));     //设置最小尺寸
    this->setMaximumSize(QSize(Widget::pos.width(), Widget::pos.height()));                 //设置最大尺寸


    ui->exitmeetBtn->setDisabled(true);             //退出按钮，设置禁用
    ui->joinmeetBtn->setDisabled(true);             //加入按钮，设置禁用
    ui->createmeetBtn->setDisabled(true);           //创建会议按钮，设置禁用
    ui->openAudio->setDisabled(true);               //开启音频按钮，设置禁用
    ui->openVedio->setDisabled(true);               //开启摄像头按钮，设置禁用
    ui->sendmsg->setDisabled(true);                 //发送按钮，设置禁用
    mainip = 0; //主屏幕显示的用户IP图像

    //-------------------局部线程----------------------------
    //创建传输视频帧线程
    _sendImg = new SendImg();
    _imgThread = new QThread();
    _sendImg->moveToThread(_imgThread); //新起线程接受视频帧     //把_sendImg移动到_imgThread线程执行
    _sendImg->start();
    //_imgThread->start();
    //处理每一帧数据

    //--------------------------------------------------


    //数据处理（局部线程）
    _mytcpSocket = new MyTcpSocket(); // 底层线程专管发送       //实例化tcp套接字
    connect(_mytcpSocket, SIGNAL(sendTextOver()), this, SLOT(textSend()));          //当接收到myTcpSocket发出的sendTextOver发送信息结束信号，主窗口执行textSend，发送
    //connect(_mytcpSocket, SIGNAL(socketerror(QAbstractSocket::SocketError)), this, SLOT(mytcperror(QAbstractSocket::SocketError)));


    //----------------------------------------------------------
    //文本传输(局部线程)
    _sendText = new SendText();
    _textThread = new QThread();                            //发送文本的线程_textThread
    _sendText->moveToThread(_textThread);                   //_sendText移到发送文本线程中执行
    _textThread->start();                                   // 加入线程
    _sendText->start();                                     // 发送

    connect(this, SIGNAL(PushText(MSG_TYPE,QString)), _sendText, SLOT(push_Text(MSG_TYPE,QString)));        //主界面发出PushText信号，SendText调用 push_Text  776和801都会调用
    //-----------------------------------------------------------

    //配置摄像头
    _camera = new QCamera(this);
    //摄像头出错处理
    connect(_camera, SIGNAL(error(QCamera::Error)), this, SLOT(cameraError(QCamera::Error)));       //摄像头如果打开失败，发送信号error,让主窗口，调用cameraError，弹出警告消息框
    _imagecapture = new QCameraImageCapture(_camera);       //截屏
    _myvideosurface = new MyVideoSurface(this);             //视频显示控件


    connect(_myvideosurface, SIGNAL(frameAvailable(QVideoFrame)), this, SLOT(cameraImageCapture(QVideoFrame)));     //视频显示控件类发出frameAvailable信号，主窗口调用cameraImageCapture视频图片处理
    connect(this, SIGNAL(pushImg(QImage)), _sendImg, SLOT(ImageCapture(QImage)));           //主窗口发出pushImg信号，Sendimg调用ImageCapture处理图片，放到队列中


    //监听_imgThread退出信号
    connect(_imgThread, SIGNAL(finished()), _sendImg, SLOT(clearImgQueue()));               //当照片处理线程发出完成信号，Sendimg调用clearImgQueue清除照片队列


    //------------------启动接收数据线程-------------------------
    _recvThread = new RecvSolve();                          //接收数据线程
    connect(_recvThread, SIGNAL(datarecv(MESG*)), this, SLOT(datasolve(MESG*)), Qt::BlockingQueuedConnection);      //接收数据线程发出datarecv数据接收信号，主窗口调用datasolve数据处理     //Qt::BlockingQueuedConnection，槽函数在控制回到接收者所在线程的事件循环时被调用，槽函数运行于信号接收者所在线程，不过在发送完信号后，发送者所在线程会阻塞，直到槽函数运行完。接收者和发送者绝对不能在一个线程，否则会死锁。在多线程间需要同步的场合会用到这个。
    _recvThread->start();                                   //启动接收数据线程

    //预览窗口重定向在MyVideoSurface
    _camera->setViewfinder(_myvideosurface);                //摄像头设置，设置取景器，取景器就是将图像实时在屏幕显示，就跟相机的屏幕一样，参数是取景器类的实例对象
    _camera->setCaptureMode(QCamera::CaptureStillImage);    //设置取景模式，静态帧捕获 也就是拍照

    //音频
    _ainput = new AudioInput();
    _ainputThread = new QThread();                          //录音的线程
    _ainput->moveToThread(_ainputThread);                   //把录音函数放入录音线程


    _aoutput = new AudioOutput();                           //播放声音
    _ainputThread->start();         //获取音频，发送
    _aoutput->start();              //播放

    connect(this, SIGNAL(startAudio()), _ainput, SLOT(startCollect()));                     //主窗口发出startAudio开始录音信号，录音类执行开始收集音频函数
    connect(this, SIGNAL(stopAudio()), _ainput, SLOT(stopCollect()));                       //当主窗口发出stopAudio停止录音信号，录音类执行停止录音函数
    connect(_ainput, SIGNAL(audioinputerror(QString)), this, SLOT(audioError(QString)));    //当录音类发出audioinputerror音频获取失败信号，主窗口调用audioError函数，弹出消息框提示音频错误
    connect(_aoutput, SIGNAL(audiooutputerror(QString)), this, SLOT(audioError(QString)));  //当播放音频类发出audiooutputerror音频播放失败信号，主窗口调用audioError函数，弹出消息框提示音频错误
    connect(_aoutput, SIGNAL(speaker(QString)), this, SLOT(speaks(QString)));               //当播放音频类发出speaker信号，主窗口调用speaks函数，outlog输出日志打印xxip正在说话

    //设置滚动条
    //设置副屏幕滚动区域的样式表
    ui->scrollArea->verticalScrollBar()->setStyleSheet("QScrollBar:vertical { width:8px; background:rgba(0,0,0,0%); margin:0px,0px,0px,0px; padding-top:9px; padding-bottom:9px; } QScrollBar::handle:vertical { width:8px; background:rgba(0,0,0,25%); border-radius:4px; min-height:20; } QScrollBar::handle:vertical:hover { width:8px; background:rgba(0,0,0,50%); border-radius:4px; min-height:20; } QScrollBar::add-line:vertical { height:9px;width:8px; border-image:url(:/images/a/3.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical { height:9px;width:8px; border-image:url(:/images/a/1.png); subcontrol-position:top; } QScrollBar::add-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/4.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/2.png); subcontrol-position:top; } QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:rgba(0,0,0,10%); border-radius:4px; }");
    //设置聊天窗口滚动区域的样式表
    ui->listWidget->setStyleSheet("QScrollBar:vertical { width:8px; background:rgba(0,0,0,0%); margin:0px,0px,0px,0px; padding-top:9px; padding-bottom:9px; } QScrollBar::handle:vertical { width:8px; background:rgba(0,0,0,25%); border-radius:4px; min-height:20; } QScrollBar::handle:vertical:hover { width:8px; background:rgba(0,0,0,50%); border-radius:4px; min-height:20; } QScrollBar::add-line:vertical { height:9px;width:8px; border-image:url(:/images/a/3.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical { height:9px;width:8px; border-image:url(:/images/a/1.png); subcontrol-position:top; } QScrollBar::add-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/4.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/2.png); subcontrol-position:top; } QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:rgba(0,0,0,10%); border-radius:4px; }");

    //设置字体
    QFont te_font = this->font();
    te_font.setFamily("MicrosoftYaHei");        //设置字体
    te_font.setPointSize(12);                   //设置字体大小

    ui->listWidget->setFont(te_font);           //聊天窗口运用该字体

    ui->tabWidget->setCurrentIndex(1);          //设置当 ComBoBox折叠后它显示的值
    ui->tabWidget->setCurrentIndex(0);
}


void Widget::cameraImageCapture(QVideoFrame frame)          //摄像头照片获取  104
{
//    qDebug() << QThread::currentThreadId() << this;

    if(frame.isValid() && frame.isReadable())               //如果视频帧frame是有效且可读的
    {
        QImage videoImg = QImage(frame.bits(), frame.width(), frame.height(), QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));        //视频图片参数（bit数，宽，高，QVideoFrame转QImage）

        QTransform matrix;                      //转换类。转换指定如何平移，缩放，剪切，旋转等
        matrix.rotate(180.0);                   //旋转180°

        QImage img =  videoImg.transformed(matrix, Qt::FastTransformation).scaled(ui->mainshow_label->size());      //videoImg调用transformed旋转（逆时针旋转）在调用scaled,根据主屏幕的尺寸返回去自适应窗口大小

        if(partner.size() > 1)      //如果房间用户的大小大于1
        {
            emit pushImg(img);      //发出信号pushImg，被sendimg类发送照片捕获，105
        }

        if(_mytcpSocket->getlocalip() == mainip)        //如果网络通信套接字 得到的本地ip=主屏幕的ip
        {
            ui->mainshow_label->setPixmap(QPixmap::fromImage(img).scaled(ui->mainshow_label->size()));      //主屏幕设置一个QPixmap图像，（利用QImage对象img去填充QPixmap,进而再调用scaled,根据主屏幕的尺寸返回去自适应窗口大小）
        }

        Partner *p = partner[_mytcpSocket->getlocalip()];       //Partner房间信息类 创建一个指针p = partner用于记录房间当前用户，其套接字指向本地ip
        if(p) p->setpic(img);                                   //指针p获取成功，则进而获取照片img

        //qDebug()<< "format: " <<  videoImg.format() << "size: " << videoImg.size() << "byteSIze: "<< videoImg.sizeInBytes();
    }
    frame.unmap();      //映射内存
}

Widget::~Widget()       //析构
{
    //终止底层发送与接收线程

    if(_mytcpSocket->isRunning())           //isRunning标志量判断线程是否关闭
    {
        _mytcpSocket->stopImmediately();    //立即停止线程
        _mytcpSocket->wait();               //线程关闭 进入等待
    }

    //终止接收处理线程
    if(_recvThread->isRunning())
    {
        _recvThread->stopImmediately();
        _recvThread->wait();
    }

    if(_imgThread->isRunning())
    {
        _imgThread->quit();
        _imgThread->wait();
    }

    if(_sendImg->isRunning())
    {
        _sendImg->stopImmediately();
        _sendImg->wait();
    }

    if(_textThread->isRunning())
    {
        _textThread->quit();
        _textThread->wait();
    }

    if(_sendText->isRunning())
    {
        _sendText->stopImmediately();
        _sendText->wait();
    }
    
    if (_ainputThread->isRunning())
    {
        _ainputThread->quit();
        _ainputThread->wait();
    }

    if (_aoutput->isRunning())
    {
        _aoutput->stopImmediately();
        _aoutput->wait();
    }
    WRITE_LOG("-------------------Application End-----------------");

    //关闭日志
    if(logqueue->isRunning())
    {
        logqueue->stopImmediately();
        logqueue->wait();
    }

    delete ui;
}

void Widget::on_createmeetBtn_clicked()         //创建按钮点击
{
    if(false == _createmeet)                    //创建会议
    {
        ui->createmeetBtn->setDisabled(true);   //创建按钮，设置不可用
        ui->openAudio->setDisabled(true);       //开启音频按钮，设置不可用
        ui->openVedio->setDisabled(true);       //开启摄像头按钮，设置不可用
        ui->exitmeetBtn->setDisabled(true);     //退出按钮，设置不可用
        emit PushText(CREATE_MEETING);  //将 “创建会议"加入到发送队列       //发送信号PushText，被sendtext发送文本类捕获   93
        WRITE_LOG("create meeting");
    }
}

void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    /*
     * 触发事件(3条， 一般使用第二条进行触发)
     * 1. 窗口部件第一次显示时，系统会自动产生一个绘图事件。从而强制绘制这个窗口部件，主窗口起来会绘制一次
     * 2. 当重新调整窗口部件的大小时，系统也会产生一个绘制事件--QWidget::update()或者QWidget::repaint()
     * 3. 当窗口部件被其它窗口部件遮挡，然后又再次显示出来时，就会对那些隐藏的区域产生一个绘制事件
    */
}


//退出会议（1，加入的会议， 2，自己创建的会议）
void Widget::on_exitmeetBtn_clicked()       //退出按钮
{
    if(_camera->status() == QCamera::ActiveStatus)      //如果摄像头 的状态 = QCamera::ActiveStatus摄像头正在工作
    {
        _camera->stop();        //停止摄像头
    }

    ui->createmeetBtn->setDisabled(true);           //创建按钮，设为不可使用
    ui->exitmeetBtn->setDisabled(true);             //退出按钮，设为不可使用
    _createmeet = false;                            //是否创建会议，设为false
    _joinmeet = false;                              //是否加入会议，设为false
    //-----------------------------------------
    //清空partner,房间用户信息
    clearPartner();

    // 关闭套接字
    //关闭socket
    _mytcpSocket->disconnectFromHost();
    _mytcpSocket->wait();

    ui->outlog->setText(tr("已退出会议"));       //输出日志 打印

    ui->connServer->setDisabled(false);         //连接按钮 ，设为不可使用
    ui->groupBox_2->setTitle(QString("主屏幕"));   //设置标题
//    ui->groupBox->setTitle(QString("副屏幕"));

    //清除聊天记录
    while(ui->listWidget->count() > 0)              //聊天窗口的 计数大于0
    {
        QListWidgetItem *item = ui->listWidget->takeItem(0);
        ChatMessage *chat = (ChatMessage *) ui->listWidget->itemWidget(item);       //创建一个聊天信息指针chat，
        delete item;
        delete chat;
    }
    iplist.clear();                                 //清除ip表
    ui->plainTextEdit->setCompleter(iplist);        //信息输入框


    WRITE_LOG("exit meeting");

    QMessageBox::warning(this, "Information", "退出会议" , QMessageBox::Yes, QMessageBox::Yes);     //弹出消息框退出会议

    //-----------------------------------------
}

void Widget::on_openVedio_clicked()                 //开启摄像头按钮
{
    if(_camera->status() == QCamera::ActiveStatus)      //如果摄像头状态正在运行
    {
        _camera->stop();                                //停止摄像头
        WRITE_LOG("close camera");      //“关闭摄像头”
        if(_camera->error() == QCamera::NoError)        //如果摄像头出错
        {
            _imgThread->quit();                         //退出照片线程
            _imgThread->wait();                         //让照片线程等待
            ui->openVedio->setText("开启摄像头");            //openVedio按钮，设置文本“开启摄像头”
            emit PushText(CLOSE_CAMERA);                   //主窗口发送 PushText信号，sendtext函数调用push_Text去输出文本  93
        }
        closeImg(_mytcpSocket->getlocalip());               //关闭本地ip的照片
    }
    else
    {
        _camera->start(); //开启摄像头
        WRITE_LOG("open camera");
        if(_camera->error() == QCamera::NoError)        //如果摄像头出错
        {
            _imgThread->start();                        //启动照片线程
            ui->openVedio->setText("关闭摄像头");        //设置openVedio按钮文本“关闭摄像头”
        }
    }
}


void Widget::on_openAudio_clicked()                     //开启音频按钮
{
    if (!_createmeet && !_joinmeet) return;             //如果没有创建会议且没有加入会议 退出
    if (ui->openAudio->text().toUtf8() == QString(OPENAUDIO).toUtf8()) //如果openAudio按钮文本是开启音频
    {
        emit startAudio();                              //发送信号 startAudio,   AudioInput类调用startCollect，开始收集音频 131
        ui->openAudio->setText(QString(CLOSEAUDIO).toUtf8());       //openAudio按钮，文本设置为CLOSEAUDIO 关闭音频，方便后续关闭
    }
    else if(ui->openAudio->text().toUtf8() == QString(CLOSEAUDIO).toUtf8()) //如果openAudio按钮文本是关闭音频
    {
        emit stopAudio();                               //发送信号 stopAudio ，AudioInput类调用stopCollect ，关闭音频
        ui->openAudio->setText(QString(OPENAUDIO).toUtf8());        //openAudio按钮，文本设置为 OPENAUDIO 打开音频，
    }
}

void Widget::closeImg(quint32 ip)                   //根据IP重置图像，关闭图像 334,580
{
    if (!partner.contains(ip))                      //如果房间用户的ip没有出现，contains判断一个指定的字符串是否出现
    {
        qDebug() << "close img error";              //打印 “关闭照片失败”
        return;
    }
    Partner * p = partner[ip];                      //房间用户ip赋值给p
    p->setpic(QImage(":/myImage/1.jpg"));           //给 p 设置图像属性

    if(mainip == ip)                                //如果主程序ip = ip
    {
        ui->mainshow_label->setPixmap(QPixmap::fromImage(QImage(":/myImage/1.jpg").scaled(ui->mainshow_label->size())));    //给主窗口屏幕设置背景，尺寸为主窗口屏幕大小
    }
}

void Widget::on_connServer_clicked()                //连接 按钮
{
    QString ip = ui->ip->text(), port = ui->port->text();       //用ip接收 ip地址框的内容；port接收端口地址框的内容
    ui->outlog->setText("正在连接到" + ip + ":" + port);         //输出日志，设置文本“正在连接到 xxip : xx端口”
    repaint();                                      //触发重绘操作，会自动调用paintEvent函数来完成对应区域的重绘

    QRegExp ipreg("((2{2}[0-3]|2[01][0-9]|1[0-9]{2}|0?[1-9][0-9]|0{0,2}[1-9])\\.)((25[0-5]|2[0-4][0-9]|[01]?[0-9]{0,2})\\.){2}(25[0-5]|2[0-4][0-9]|[01]?[0-9]{1,2})");

    QRegExp portreg("^([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])$");     //正则表达式
    QRegExpValidator ipvalidate(ipreg), portvalidate(portreg);      //QRegExpValidator 使用正则表达式 （ipreg,portreg） 来确定输入字符串是可接受、中间还是无效的。
    int pos = 0;
    if(ipvalidate.validate(ip, pos) != QValidator::Acceptable)      //如果ip输入验证，!= 可接受
    {
        QMessageBox::warning(this, "Input Error", "Ip Error", QMessageBox::Yes, QMessageBox::Yes);      //消息框警告，ip输入错误
        return;
    }
    if(portvalidate.validate(port, pos) != QValidator::Acceptable)
    {
        QMessageBox::warning(this, "Input Error", "Port Error", QMessageBox::Yes, QMessageBox::Yes);
        return;
    }

    if(_mytcpSocket ->connectToServer(ip, port, QIODevice::ReadWrite))      //tcp套接字连接（ip,端口，文件写入模式：读写）
    {
        ui->outlog->setText("成功连接到" + ip + ":" + port);              //输出日志
        ui->openAudio->setDisabled(true);                               //打开音频按钮，设置不可使用
        ui->openVedio->setDisabled(true);                               //打开视频按钮，~
        ui->createmeetBtn->setDisabled(false);                          //创建会议按钮，~
        ui->exitmeetBtn->setDisabled(true);                             //退出会议按钮，~
        ui->joinmeetBtn->setDisabled(false);                            //加入会议按钮，~
        WRITE_LOG("succeeed connecting to %s:%s", ip.toStdString().c_str(), port.toStdString().c_str());    //日志：成功连接到 xxip的xx端口
        QMessageBox::warning(this, "Connection success", "成功连接服务器" , QMessageBox::Yes, QMessageBox::Yes);
        ui->sendmsg->setDisabled(false);                                //发送信息按钮，设置为可使用
        ui->connServer->setDisabled(true);                              //连接按钮，设置不可使用
    }
    else
    {
        ui->outlog->setText("连接失败,请重新连接...");
        WRITE_LOG("failed to connenct %s:%s", ip.toStdString().c_str(), port.toStdString().c_str());
        QMessageBox::warning(this, "Connection error", _mytcpSocket->errorString() , QMessageBox::Yes, QMessageBox::Yes);
    }
}


void Widget::cameraError(QCamera::Error)    //99
{
    QMessageBox::warning(this, "Camera error", _camera->errorString() , QMessageBox::Yes, QMessageBox::Yes);        //警告消息提示框，摄像头打开失败
}

void Widget::audioError(QString err)        //133，134
{
    QMessageBox::warning(this, "Audio error", err, QMessageBox::Yes);           //警告消息提示框，音频错误
}

void Widget::datasolve(MESG *msg)           //数据处理，信息   114
{
    if(msg->msg_type == CREATE_MEETING_RESPONSE)        //如果信息的消息结构体 = CREATE_MEETING_RESPONSE
    {
        int roomno;
        memcpy(&roomno, msg->data, msg->len);           //用于把资源内存（msg->data所指向的内存区域） 拷贝(msg->len)个字节 到目标内存（&roomno所指向的内存区域）

        if(roomno != 0)
        {
            QMessageBox::information(this, "Room No", QString("房间号：%1").arg(roomno), QMessageBox::Yes, QMessageBox::Yes);

            ui->groupBox_2->setTitle(QString("主屏幕(房间号: %1)").arg(roomno));      //设置主屏幕标题
            ui->outlog->setText(QString("创建成功 房间号: %1").arg(roomno) );          //日志输出“创建成功 房间号xx”
            _createmeet = true;                                                 //是否创建会议置为true
            ui->exitmeetBtn->setDisabled(false);            //退出会议按钮，可用
            ui->openVedio->setDisabled(false);              //打开视频按钮，可用
            ui->joinmeetBtn->setDisabled(true);             //加入会议按钮， 设置为不可用

            WRITE_LOG("succeed creating room %d", roomno);      //日志记录
            //添加用户自己
            addPartner(_mytcpSocket->getlocalip());             //增加用户，tcp套接字获取本地ip
            mainip = _mytcpSocket->getlocalip();                //主程序ip = 套接字获取的本地ip
            ui->groupBox_2->setTitle(QHostAddress(mainip).toString());      //设置主屏幕标题为 主程序ip
            ui->mainshow_label->setPixmap(QPixmap::fromImage(QImage(":/myImage/1.jpg").scaled(ui->mainshow_label->size())));    //设置主屏幕背景图片
        }
        else
        {
            _createmeet = false;                                                //是否创建会议置为flase
            QMessageBox::information(this, "Room Information", QString("无可用房间"), QMessageBox::Yes, QMessageBox::Yes);       //消息框提示 无可用房间
            ui->outlog->setText(QString("无可用房间"));
            ui->createmeetBtn->setDisabled(false);
            WRITE_LOG("no empty room");
        }
    }

    else if(msg->msg_type == JOIN_MEETING_RESPONSE)          //如果信息的消息结构体 = JOIN_MEETING_RESPONSE
    {
        qint32 c;
        memcpy(&c, msg->data, msg->len);                     //用于把资源内存（msg->data所指向的内存区域） 拷贝(msg->len)个字节 到目标内存（&c所指向的内存区域）
        if(c == 0)
        {
            //会议不存在
            QMessageBox::information(this, "Meeting Error", tr("会议不存在") , QMessageBox::Yes, QMessageBox::Yes);
            ui->outlog->setText(QString("会议不存在"));
            WRITE_LOG("meeting not exist");
            ui->exitmeetBtn->setDisabled(true);     //退出会议按钮，不可用
            ui->openVedio->setDisabled(true);       //打开视频按钮，不可用
            ui->joinmeetBtn->setDisabled(false);        //加入会议按钮，可用
            ui->connServer->setDisabled(true);      //连接按钮，不可用
            _joinmeet = false;          //加入会议置为false
        }
        else if(c == -1)
        {
            //会议不符合，无法加入
            QMessageBox::warning(this, "Meeting information", "成员已满，无法加入" , QMessageBox::Yes, QMessageBox::Yes);
            ui->outlog->setText(QString("成员已满，无法加入"));
            WRITE_LOG("full room, cannot join");
        }
        else if (c > 0)
        {
            //成功加入会议
            QMessageBox::warning(this, "Meeting information", "加入成功" , QMessageBox::Yes, QMessageBox::Yes);
            ui->outlog->setText(QString("加入成功"));
            WRITE_LOG("succeed joining room");

            //添加用户自己
            addPartner(_mytcpSocket->getlocalip());         //增加用户，tcp套接字获取本地ip
            mainip = _mytcpSocket->getlocalip();            //主程序ip = 套接字获取的本地ip
            ui->groupBox_2->setTitle(QHostAddress(mainip).toString());          //主屏幕设置标题，主程序ip
            ui->mainshow_label->setPixmap(QPixmap::fromImage(QImage(":/myImage/1.jpg").scaled(ui->mainshow_label->size())));        //主屏幕背景
            ui->joinmeetBtn->setDisabled(true);             //加入会议按钮，设置为不可用
            ui->exitmeetBtn->setDisabled(false);            //退出会议按钮，可用
            ui->createmeetBtn->setDisabled(true);           //创建会议按钮，设置为不可用
            _joinmeet = true;                         //是否加入会议，置为true
        }
    }

    else if(msg->msg_type == IMG_RECV)                      //如果信息的消息结构体 = IMG_RECV
    {
        QHostAddress a(msg->ip);
        qDebug() << a.toString();
        QImage img;
        img.loadFromData(msg->data, msg->len);              //根据内存中的图像数据进行导入
        if(partner.count(msg->ip) == 1)                     //如果用户的数量为1
        {
            Partner* p = partner[msg->ip];
            p->setpic(img);                                 //给 指针p 设置图像
        }
        else
        {
            Partner* p = addPartner(msg->ip);               //指针p 赋增加用户
            p->setpic(img);
        }

        if(msg->ip == mainip)                               //如果信息指向的ip =主程序ip
        {
            ui->mainshow_label->setPixmap(QPixmap::fromImage(img).scaled(ui->mainshow_label->size()));
        }
        repaint();
    }

    else if(msg->msg_type == TEXT_RECV)                     //如果信息的消息结构体 = IMG_RECV
    {
        QString str = QString::fromStdString(std::string((char *)msg->data, msg->len));     //从std::string 转为QString
        //qDebug() << str;
        QString time = QString::number(QDateTime::currentDateTimeUtc().toTime_t());         //用QDateTime获得本地时间
        ChatMessage *message = new ChatMessage(ui->listWidget);                             //聊天信息
        QListWidgetItem *item = new QListWidgetItem();
        dealMessageTime(time);                                      //处理时间 815
        dealMessage(message, item, str, time, QHostAddress(msg->ip).toString() ,ChatMessage::User_She);     //处理信息 800
        if(str.contains('@' + QHostAddress(_mytcpSocket->getlocalip()).toString()))         //判断@xxx 是否出现过
        {
            QSound::play(":/myEffect/2.wav");               //对平台音频设备的访问
        }
    }
    else if(msg->msg_type == PARTNER_JOIN)                  //如果信息的消息结构体 = PARTNER_JOIN
    {
        Partner* p = addPartner(msg->ip);           //增加用户
        if(p)
        {
            p->setpic(QImage(":/myImage/1.jpg"));
            ui->outlog->setText(QString("%1 join meeting").arg(QHostAddress(msg->ip).toString()));
            iplist.append(QString("@") + QHostAddress(msg->ip).toString());     //把@xxip 追加到 iplist后面
            ui->plainTextEdit->setCompleter(iplist);                            //聊天窗口输入框
        }
    }
    else if(msg->msg_type == PARTNER_EXIT)                  //如果信息的消息结构体 = PARTNER_EXIT
    {
        removePartner(msg->ip);                             //移除用户
        if(mainip == msg->ip)                               //如果主程序ip = 信息指向的ip
        {
            ui->mainshow_label->setPixmap(QPixmap::fromImage(QImage(":/myImage/1.jpg").scaled(ui->mainshow_label->size())));
        }
        if(iplist.removeOne(QString("@") + QHostAddress(msg->ip).toString()))       //removeOne通过元素值删除元素，需要指定要删除的元素值
        {
            ui->plainTextEdit->setCompleter(iplist);
        }
        else
        {
            qDebug() << QHostAddress(msg->ip).toString() << "not exist";            //不存在
            WRITE_LOG("%s not exist",QHostAddress(msg->ip).toString().toStdString().c_str());
        }
        ui->outlog->setText(QString("%1 exit meeting").arg(QHostAddress(msg->ip).toString()));      //日志输出 “xxip 退出会议”
    }
    else if (msg->msg_type == CLOSE_CAMERA)                 //如果信息的消息结构体 = CLOSE_CAMERA
    {
        closeImg(msg->ip);                                  //关闭图像
    }
    else if (msg->msg_type == PARTNER_JOIN2)                //如果信息的消息结构体 = PARTNER_JOIN2
    {
        uint32_t ip;
        int other = msg->len / sizeof(uint32_t), pos = 0;           //sizeof(uint32_t) = 4字节
        for (int i = 0; i < other; i++)
        {
            memcpy_s(&ip, sizeof(uint32_t), msg->data + pos , sizeof(uint32_t));        //？？复制src指向的对象中的字符到dest指向的对象。 两个对象都被解释为无符号字符数组。
            pos += sizeof(uint32_t);                        //pos计数+4字节
            Partner* p = addPartner(ip);                    //增加用户ip
            if (p)
            {
                p->setpic(QImage(":/myImage/1.jpg"));       //设置图像
                iplist << QString("@") + QHostAddress(ip).toString();                   //？list
            }
        }
        ui->plainTextEdit->setCompleter(iplist);                //文本编辑框
        ui->openVedio->setDisabled(false);                      //打开视频按钮，设为可用
    }
    else if(msg->msg_type == RemoteHostClosedError)         //如果信息的消息结构体 = RemoteHostClosedError
    {

        clearPartner();                                     //清除用户记录
        _mytcpSocket->disconnectFromHost();                 //tcp套接字线程断开从主机的连接
        _mytcpSocket->wait();
        ui->outlog->setText(QString("关闭与服务器的连接"));     //日志输出“关闭与服务器连接”
        ui->createmeetBtn->setDisabled(true);               //创建会议按钮，不可用
        ui->exitmeetBtn->setDisabled(true);                 //退出会议按钮，不可用
        ui->connServer->setDisabled(false);                 //连接按钮，设为可用
        ui->joinmeetBtn->setDisabled(true);                 //加入会议按钮，不可用
        //清除聊天记录
        while(ui->listWidget->count() > 0)                  //聊天窗口行数>0 就一直清除
        {
            QListWidgetItem *item = ui->listWidget->takeItem(0);            //将QListWidgetItem从QListWidget列表中删除，使用takeItem但要知道删除的是第几个Item。而且返回删除的Item指针
            ChatMessage *chat = (ChatMessage *)ui->listWidget->itemWidget(item);
            delete item;
            delete chat;
        }
        iplist.clear();
        ui->plainTextEdit->setCompleter(iplist);            //??
        if(_createmeet || _joinmeet) QMessageBox::warning(this, "Meeting Information", "会议结束" , QMessageBox::Yes, QMessageBox::Yes);
    }
    else if(msg->msg_type == OtherNetError)                 //如果信息的消息结构体 = OtherNetError
    {
        QMessageBox::warning(NULL, "Network Error", "网络异常" , QMessageBox::Yes, QMessageBox::Yes);       //消息框警告网络错误
        clearPartner();                                     //清除用户
        _mytcpSocket->disconnectFromHost();                 //tcp套接字断开主机连接
        _mytcpSocket->wait();                               //wait等待，防止断开时仍有线程未运行完，等待一会在断开
        ui->outlog->setText(QString("网络异常......"));
    }
    if(msg->data)
    {
        free(msg->data);                    //free释放指针所指向地址的空间，本质上就是做了一些标记而已，所以指针及空间内容都还是存在的,存在隐患可能变成野指针
        msg->data = NULL;                   //所以free释放地址空间后，应该把指针指向NULL
    }
    if(msg)
    {
        free(msg);
        msg = NULL;
    }
}

Partner* Widget::addPartner(quint32 ip)                     //增加用户信息    454,500,551,590
{
    if (partner.contains(ip)) return NULL;                  //如果ip已经存在，退出
    Partner *p = new Partner(ui->scrollAreaWidgetContents ,ip);     //？
    if (p == NULL)
    {
        qDebug() << "new Partner error";                    //创建新用户失败
        return NULL;
    }
    else
    {
        connect(p, SIGNAL(sendip(quint32)), this, SLOT(recvip(quint32)));       //p发出信号sendip发送ip时，主窗口recvip接收ip
        partner.insert(ip, p);                              //map容器partner 插入ip,p
        ui->verticalLayout_3->addWidget(p, 1);              //增加窗口（p,所在行1）

		//当有人员加入时，开启滑动条滑动事件，开启输入(只有自己时，不打开)
        if (partner.size() > 1)                             //如果房间用户 数量 > 1
        {
            //主窗口发出信号volumnChange，AudioInput和AudioOutput调用setVolumn ；参数Qt::UniqueConnection：此参数保证同一个信号和槽函数只会绑定一次。
            connect(this, SIGNAL(volumnChange(int)), _ainput, SLOT(setVolumn(int)), Qt::UniqueConnection);
			connect(this, SIGNAL(volumnChange(int)), _aoutput, SLOT(setVolumn(int)), Qt::UniqueConnection);

            ui->openAudio->setDisabled(false);              //打开音频按钮，设为可用
            ui->sendmsg->setDisabled(false);                //发送图像按钮，设为可用
            _aoutput->startPlay();                          //播放音频
        }
		return p;
    }
}

void Widget::removePartner(quint32 ip)                      //移除用户信息    562
{
    if(partner.contains(ip))                                //如果ip存在
    {
        Partner *p = partner[ip];
        disconnect(p, SIGNAL(sendip(quint32)), this, SLOT(recvip(quint32)));        //取消槽函数，654
        ui->verticalLayout_3->removeWidget(p);              //移除窗口p
        delete p;
        partner.remove(ip);

        //只有自已一个人时，关闭传输音频
        if (partner.size() <= 1)
        {
            disconnect(_ainput, SLOT(setVolumn(int)));      //取消槽  662
            disconnect(_aoutput, SLOT(setVolumn(int)));     //       663
            _ainput->stopCollect();                         //停止音频采集
            _aoutput->stopPlay();                           //停止音频播放
            ui->openAudio->setText(QString(OPENAUDIO).toUtf8());        //设置openAudio按钮，文本 OPENAUDIO
            ui->openAudio->setDisabled(true);               //打开音频按钮，设为不可用
        }
    }
}

void Widget::clearPartner()                                 //清除用户信息        289,603,626
{
    ui->mainshow_label->setPixmap(QPixmap());
    if(partner.size() == 0) return;                         //如果用户数量=0，直接退出

    QMap<quint32, Partner*>::iterator iter =   partner.begin();
    while (iter != partner.end())                   //iter != map容器的末尾
    {
        quint32 ip = iter.key();                    //partner容器(ip, p)，key值是ip
        iter++;
        Partner *p =  partner.take(ip);             //take：从这个容器中将对象删除，并返回之前放在这个容器中的对象或者数据
        ui->verticalLayout_3->removeWidget(p);      //移除返回的ip
        delete p;                                   //先delete，在置nullptr
        p = nullptr;
    }

    //关闭传输音频
    disconnect(_ainput, SLOT(setVolumn(int)));      //取消槽函数 ，662
    disconnect(_aoutput, SLOT(setVolumn(int)));     //           663
    //关闭音频播放与采集
    _ainput->stopCollect();                         //关闭音频采集
    _aoutput->stopPlay();                           //关闭音频播放
    ui->openAudio->setText(QString(CLOSEAUDIO).toUtf8());           //设置openAudio按钮文本 CLOSEAUDIO
    ui->openAudio->setDisabled(true);               //打开音频按钮，设为不可用
    

    //关闭图片传输线程
    if(_imgThread->isRunning())                     //如果图片线程 仍在运行
    {
        _imgThread->quit();                         //关闭
        _imgThread->wait();                         //等待一会
    }
    ui->openVedio->setText(QString(OPENVIDEO).toUtf8());            //设置openVedio按钮，文本 OPENVIDEO
    ui->openVedio->setDisabled(true);               //打开视频按钮，设为不可用
}

void Widget::recvip(quint32 ip)                     //接收ip  654,678
{
    if (partner.contains(mainip))                   //如果用户信息中存在mainip
    {
        Partner* p = partner[mainip];               //mainip赋给指针p
        p->setStyleSheet("border-width: 1px; border-style: solid; border-color:rgba(0, 0 , 255, 0.7)");     //样式表
    }
    if (partner.contains(ip))                       //如果用户信息中存在ip
	{
        Partner* p = partner[ip];
		p->setStyleSheet("border-width: 1px; border-style: solid; border-color:rgba(255, 0 , 0, 0.7)");
	}

    ui->mainshow_label->setPixmap(QPixmap::fromImage(QImage(":/myImage/1.jpg").scaled(ui->mainshow_label->size())));    //主屏幕背景
    mainip = ip;                                    //？ip的值赋给mainip
    ui->groupBox_2->setTitle(QHostAddress(mainip).toString());          //主屏幕设置标题mianip
    qDebug() << ip;
}

/*
 *          加入会议
 */

void Widget::on_joinmeetBtn_clicked()               //加入会议按钮
{
    QString roomNo = ui->meetno->text();            //会议编号的文本 赋给roomNo

    QRegExp roomreg("^[1-9][0-9]{1,4}$");           //正则表达式
    QRegExpValidator  roomvalidate(roomreg);        //正则表达式检查字符串

    int pos = 0;
    if(roomvalidate.validate(roomNo, pos) != QValidator::Acceptable)        //如果正则表达式检查的字符串 != 允许的字符
    {
        QMessageBox::warning(this, "RoomNo Error", "房间号不合法" , QMessageBox::Yes, QMessageBox::Yes);      //消息框警告 不合法
    }
    else
    {
        //加入发送队列
        emit PushText(JOIN_MEETING, roomNo);        //发送PushText信号，SendText类调用push_Text 93
    }
}

void Widget::on_horizontalSlider_valueChanged(int value)        //音量进度条，值发生改变
{
    emit volumnChange(value);                       //发出信号，volumnChange，音频输入输出类调用setVolumn 662，663
}

void Widget::speaks(QString ip)                     //？？说话      135
{
    ui->outlog->setText(QString(ip + " 正在说话").toUtf8());        //日志输出，xxip正在说话
}

void Widget::on_sendmsg_clicked()                   //发送消息按钮
{
    QString msg = ui->plainTextEdit->toPlainText().trimmed();       //将文本编辑框的内容去掉@去掉尾部返回；trimmed:返回值为去除了开头和结尾的空白字符串
    if(msg.size() == 0)                             //如果信息大小为0
    {
        qDebug() << "empty";                        //点击发生按钮，打印empty
        return;
    }

    //信息正常发送，大小不为0
    qDebug()<<msg;                          //点击发送按钮后，打印出文本信息
    ui->plainTextEdit->setPlainText("");
    QString time = QString::number(QDateTime::currentDateTimeUtc().toTime_t());     //获取本地时间
    ChatMessage *message = new ChatMessage(ui->listWidget);
    QListWidgetItem *item = new QListWidgetItem();
    dealMessageTime(time);                  //处理文本时间
    dealMessage(message, item, msg, time, QHostAddress(_mytcpSocket->getlocalip()).toString() ,ChatMessage::User_Me);       //处理文本信息
    emit PushText(TEXT_SEND, msg);          //发送PushText信号，SendText类调用push_Text 93
    ui->sendmsg->setDisabled(true);         //发送信息按钮，设为不可用
}

void Widget::dealMessage(ChatMessage *messageW, QListWidgetItem *item, QString text, QString time, QString ip ,ChatMessage::User_Type type)         //处理用户发送的文本信息   543,800
{
    ui->listWidget->addItem(item);          //在聊天窗口添加文本 item
    messageW->setFixedWidth(ui->listWidget->width());       //文本显示宽度
    QSize size = messageW->fontRect(text);                  //设置字体格式
    item->setSizeHint(size);                                //设置窗口组件尺寸
    messageW->setText(text, time, size, ip, type);          //messageW的文本设置
    ui->listWidget->setItemWidget(item, messageW);          //设置聊天窗口的组件
}

void Widget::dealMessageTime(QString curMsgTime)            //处理时间 542,799
{
    bool isShowTime = false;                                //是否显示时间
    if(ui->listWidget->count() > 0)
    {
        QListWidgetItem* lastItem = ui->listWidget->item(ui->listWidget->count() - 1);
        ChatMessage* messageW = (ChatMessage *)ui->listWidget->itemWidget(lastItem);
        int lastTime = messageW->time().toInt();
        int curTime = curMsgTime.toInt();
        qDebug() << "curTime lastTime:" << curTime - lastTime;
        isShowTime = ((curTime - lastTime) > 60); // 两个消息相差一分钟
//        isShowTime = true;
    } else {
        isShowTime = true;
    }
    if(isShowTime) {
        ChatMessage* messageTime = new ChatMessage(ui->listWidget);
        QListWidgetItem* itemTime = new QListWidgetItem();
        ui->listWidget->addItem(itemTime);
        QSize size = QSize(ui->listWidget->width() , 40);
        messageTime->resize(size);
        itemTime->setSizeHint(size);                                //设置窗口组件尺寸
        messageTime->setText(curMsgTime, curMsgTime, size);         //设置时间文本
        ui->listWidget->setItemWidget(itemTime, messageTime);       //设置自定义窗口
    }
}

void Widget::textSend()                         //信息发送  w81
{
    qDebug() << "send text over";               //打印“发送信息结束”
    QListWidgetItem* lastItem = ui->listWidget->item(ui->listWidget->count() - 1);
    ChatMessage* messageW = (ChatMessage *)ui->listWidget->itemWidget(lastItem);
    messageW->setTextSuccess();                 //调用 设置信息成功发送
    ui->sendmsg->setDisabled(false);            //发送按钮，设置为可用
}
