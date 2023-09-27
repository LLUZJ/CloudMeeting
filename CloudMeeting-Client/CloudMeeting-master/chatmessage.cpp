//聊天信息

#include "chatmessage.h"
#include <QFontMetrics>     //提供关于字体的指标信息
#include <QPaintEvent>
#include <QDateTime>
#include <QPainter>
#include <QMovie>           //动画播放
#include <QLabel>
#include <QDebug>


ChatMessage::ChatMessage(QWidget *parent) : QWidget(parent)
{
    QFont te_font = this->font();
    te_font.setFamily("MicrosoftYaHei");                //设置字体 微软雅黑
    te_font.setPointSize(12);                           //字体大小 12
    this->setFont(te_font);                             //应用这个字体
    m_leftPixmap = QPixmap(":/myImage/1.jpg");
    m_rightPixmap = QPixmap(":/myImage/1.jpg");

    m_loadingMovie = new QMovie(this);
    m_loadingMovie->setFileName(":/myImage/3.gif");     //登录播放
    m_loading = new QLabel(this);
    m_loading->setMovie(m_loadingMovie);                //给登录界面设置播放
    m_loading->setScaledContents(true);                 //设置图片自适应控件大小
    m_loading->resize(40,40);                           //设置登录界面label大小40*40
    m_loading->setAttribute(Qt::WA_TranslucentBackground , true);       //设置背景透明化；  setAttribute用于设置各种窗口小部件（widget）的属性
}

void ChatMessage::setTextSuccess()      //w847
{
    m_loading->hide();              //隐藏
    m_loadingMovie->stop();         //停止
    m_isSending = true;             //发送中置true
}

void ChatMessage::setText(QString text, QString time, QSize allSize, QString ip,ChatMessage::User_Type userType)        //设置文本  w811,837
{
    m_msg = text;
    m_userType = userType;
    m_time = time;
    m_curTime = QDateTime::fromTime_t(time.toInt()).toString("ddd hh:mm");      //设置时间格式
    m_allSize = allSize;
    m_ip = ip;

    if(userType == User_Me)             //如果使用者类型是自己
    {
        if(!m_isSending)                //如果不处于m_isSending为true时；   35
        {
            m_loading->move(m_kuangRightRect.x() - m_loading->width() - 10, m_kuangRightRect.y()+m_kuangRightRect.height()/2- m_loading->height()/2);       //登录label移动
//            m_loading->move(0, 0);
            m_loading->show();          //显示登录界面
            m_loadingMovie->start();    //登录界面播放开始
        }
    } else {
        m_loading->hide();              //隐藏登录界面
    }

    this->update();
}

QSize ChatMessage::fontRect(QString str)        //字体格式设置    w809
{
    m_msg = str;
    int minHei = 30;
    int iconWH = 40;
    int iconSpaceW = 20;
    int iconRectW = 5;
    int iconTMPH = 10;
    int sanJiaoW = 6;
    int kuangTMP = 20;
    int textSpaceRect = 12;
    m_kuangWidth = this->width() - kuangTMP - 2*(iconWH+iconSpaceW+iconRectW);
    m_textWidth = m_kuangWidth - 2*textSpaceRect;
    m_spaceWid = this->width() - m_textWidth;
    m_iconLeftRect = QRect(iconSpaceW, iconTMPH + 10, iconWH, iconWH);
    m_iconRightRect = QRect(this->width() - iconSpaceW - iconWH, iconTMPH + 10, iconWH, iconWH);


    QSize size = getRealString(m_msg);          // 整个的size

    qDebug() << "fontRect Size:" << size;
    int hei = size.height() < minHei ? minHei : size.height();

    m_sanjiaoLeftRect = QRect(iconWH+iconSpaceW+iconRectW, m_lineHeight/2 + 10, sanJiaoW, hei - m_lineHeight);
    m_sanjiaoRightRect = QRect(this->width() - iconRectW - iconWH - iconSpaceW - sanJiaoW, m_lineHeight/2+10, sanJiaoW, hei - m_lineHeight);

    if(size.width() < (m_textWidth+m_spaceWid)) {
        m_kuangLeftRect.setRect(m_sanjiaoLeftRect.x()+m_sanjiaoLeftRect.width(), m_lineHeight/4*3 + 10, size.width()-m_spaceWid+2*textSpaceRect, hei-m_lineHeight);
        m_kuangRightRect.setRect(this->width() - size.width() + m_spaceWid - 2*textSpaceRect - iconWH - iconSpaceW - iconRectW - sanJiaoW,
                                 m_lineHeight/4*3 + 10, size.width()-m_spaceWid+2*textSpaceRect, hei-m_lineHeight);
    } else {
        m_kuangLeftRect.setRect(m_sanjiaoLeftRect.x()+m_sanjiaoLeftRect.width(), m_lineHeight/4*3 + 10, m_kuangWidth, hei-m_lineHeight);
        m_kuangRightRect.setRect(iconWH + kuangTMP + iconSpaceW + iconRectW - sanJiaoW, m_lineHeight/4*3 + 10, m_kuangWidth, hei-m_lineHeight);
    }
    m_textLeftRect.setRect(m_kuangLeftRect.x()+textSpaceRect, m_kuangLeftRect.y()+iconTMPH,
                           m_kuangLeftRect.width()-2*textSpaceRect, m_kuangLeftRect.height()-2*iconTMPH);
    m_textRightRect.setRect(m_kuangRightRect.x()+textSpaceRect, m_kuangRightRect.y()+iconTMPH,
                            m_kuangRightRect.width()-2*textSpaceRect, m_kuangRightRect.height()-2*iconTMPH);


    m_ipLeftRect.setRect(m_kuangLeftRect.x(), m_kuangLeftRect.y()+iconTMPH - 20,
                           m_kuangLeftRect.width()-2*textSpaceRect + iconWH*2, 20);
    m_ipRightRect.setRect(m_kuangRightRect.x(), m_kuangRightRect.y()+iconTMPH - 30,
                            m_kuangRightRect.width()-2*textSpaceRect + iconWH*2 , 20);
    return QSize(size.width(), hei + 15);
}

QSize ChatMessage::getRealString(QString src)               //得到实际字符
{
    QFontMetricsF fm(this->font());                         //QFontMetricsF 提供关于字体的指标信息
    m_lineHeight = fm.lineSpacing();                        //lineSpacing两基线之间的距离
    int nCount = src.count("\n");
    int nMaxWidth = 0;
    if(nCount == 0)
    {
        nMaxWidth = fm.width(src);                          //最大宽度 = fm的宽度
        QString value = src;
        if(nMaxWidth > m_textWidth)                         //如果最大宽度 > 文本宽度
        {
            nMaxWidth = m_textWidth;                        //令宽度相等
            int size = m_textWidth / fm.width(" ");         //设置 size = 文本宽度 / fm宽度
            int num = fm.width(value) / m_textWidth;
            num = ( fm.width(value) ) / m_textWidth;
            nCount += num;
            QString temp = "";
            for(int i = 0; i < num; i++)
            {
                temp += value.mid(i*size, (i+1)*size) + "\n";
            }
            src.replace(value, temp);                       //replace 字符替换
        }
    }
    else
    {
        for(int i = 0; i < (nCount + 1); i++)
        {
            QString value = src.split("\n").at(i);          //split 以某个字符为标志位将字符串截取成一个集合
            nMaxWidth = fm.width(value) > nMaxWidth ? fm.width(value) : nMaxWidth;
            if(fm.width(value) > m_textWidth)
            {
                nMaxWidth = m_textWidth;
                int size = m_textWidth / fm.width(" ");
                int num = fm.width(value) / m_textWidth;
                num = ((i+num)*fm.width(" ") + fm.width(value)) / m_textWidth;
                nCount += num;
                QString temp = "";
                for(int i = 0; i < num; i++)
                {
                    temp += value.mid(i*size, (i+1)*size) + "\n";
                }
                src.replace(value, temp);
            }
        }
    }
    return QSize(nMaxWidth+m_spaceWid, (nCount + 1) * m_lineHeight+2*m_lineHeight);
}

void ChatMessage::paintEvent(QPaintEvent *event)            //重写paintEvent处理绘图事件
{
    Q_UNUSED(event);

    QPainter painter(this);                         //画家painter
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);   //消锯齿
    painter.setPen(Qt::NoPen);                      //画家设置画笔，不用笔
    painter.setBrush(QBrush(Qt::gray));             //画家设置画刷，灰色填充

    if(m_userType == User_Type::User_She)           //如果使用者是 用户
    {
        //---------------------头像---------------------
        //painter.drawRoundedRect(m_iconLeftRect,m_iconLeftRect.width(),m_iconLeftRect.height());
        painter.drawPixmap(m_iconLeftRect, m_leftPixmap);       //绘图 77，78

        //框加边
        QColor col_KuangB(234, 234, 234);           //框边颜色
        painter.setBrush(QBrush(col_KuangB));       //画家设置画刷，框边色填充
        painter.drawRoundedRect(m_kuangLeftRect.x()-1,m_kuangLeftRect.y()-1 + 10,m_kuangLeftRect.width()+2,m_kuangLeftRect.height()+2,4,4);         //绘制圆角矩形
        //框
        QColor col_Kuang(255,255,255);              //框颜色
        painter.setBrush(QBrush(col_Kuang));        //画家设置画刷，框色填充
        painter.drawRoundedRect(m_kuangLeftRect,4,4);   //drawRoundedRect绘制圆角矩形

        //三角
        QPointF points[3] = {                       //三角形三点坐标
            QPointF(m_sanjiaoLeftRect.x(), 40),
            QPointF(m_sanjiaoLeftRect.x()+m_sanjiaoLeftRect.width(), 35),
            QPointF(m_sanjiaoLeftRect.x()+m_sanjiaoLeftRect.width(), 45),
        };
        QPen pen;
        pen.setColor(col_Kuang);                    //笔颜色为 框色
        painter.setPen(pen);                        //画家设置画笔 pen
        painter.drawPolygon(points, 3);             //根据三个点画多边形，drawPolygon绘制不规则多边形

        //三角加边
        QPen penSanJiaoBian;                        //笔三角边
        penSanJiaoBian.setColor(col_KuangB);        //三角边笔 颜色为框边色
        painter.setPen(penSanJiaoBian);             //画家设置画笔 三角边笔
        painter.drawLine(QPointF(m_sanjiaoLeftRect.x() - 1, 30), QPointF(m_sanjiaoLeftRect.x()+m_sanjiaoLeftRect.width(), 24));     //画线
        painter.drawLine(QPointF(m_sanjiaoLeftRect.x() - 1, 30), QPointF(m_sanjiaoLeftRect.x()+m_sanjiaoLeftRect.width(), 36));

        //ip
        //---------------------ip---------------------
        QPen penIp;                                                 //笔ip
        penIp.setColor(Qt::darkGray);                               //设置ip笔 颜色为深灰
        painter.setPen(penIp);                                      //画家设置笔为 ip笔
        QFont f = this->font();                                     //设置字体 f    15
        f.setPointSize(10);                                         //setPointSize：字体高度占用的可物理测量的长度磅（point）pt大小，一般用于印刷领域
        QTextOption op(Qt::AlignHCenter | Qt::AlignVCenter);        //文本选项 设置水平和垂直居中
        painter.setFont(f);                                         //画家设置 字体为f
        painter.drawText(m_ipLeftRect, m_ip, op);                   //画家设置 画文本，在（m_ipLeftRect, m_ip）处以op（水平和垂直居中）格式画文字

        //---------------------内容---------------------
        QPen penText;                                                       //笔文本
        penText.setColor(QColor(51,51,51));                                 //文本笔 设置颜色
        painter.setPen(penText);                                            //画家设置笔为 文本笔
        QTextOption option(Qt::AlignLeft | Qt::AlignVCenter);               //文本选项 设置水平靠右，垂直居中
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);      //设置换行模式 ： 在单词边界处进行换行
        painter.setFont(this->font());                                      //设置字体 15
        painter.drawText(m_textLeftRect, m_msg,option);                     //画文字
    }
    else if(m_userType == User_Type::User_Me)               //如果使用者是 自己
    {
        //头像
        //painter.drawRoundedRect(m_iconRightRect,m_iconRightRect.width(),m_iconRightRect.height());
        painter.drawPixmap(m_iconRightRect, m_rightPixmap);                 //设置图片

        //框
        QColor col_Kuang(75,164,242);
        painter.setBrush(QBrush(col_Kuang));
        painter.drawRoundedRect(m_kuangRightRect,4,4);

        //三角
        QPointF points[3] = {
            QPointF(m_sanjiaoRightRect.x()+m_sanjiaoRightRect.width(), 40),
            QPointF(m_sanjiaoRightRect.x(), 35),
            QPointF(m_sanjiaoRightRect.x(), 45),
        };
        QPen pen;
        pen.setColor(col_Kuang);
        painter.setPen(pen);
        painter.drawPolygon(points, 3);                         //根据三个点画多边形

        //ip
        QPen penIp;
        penIp.setColor(Qt::black);                              //设置颜色 黑色
        painter.setPen(penIp);
        QFont f = this->font();
        f.setPointSize(10);
        QTextOption op(Qt::AlignHCenter | Qt::AlignVCenter);
        painter.setFont(f);
        painter.drawText(m_ipRightRect, m_ip, op);

        //内容
        QPen penText;
        penText.setColor(Qt::white);                            //设置颜色白色
        painter.setPen(penText);
        QTextOption option(Qt::AlignLeft | Qt::AlignVCenter);
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        painter.setFont(this->font());
        painter.drawText(m_textRightRect,m_msg,option);
    }
    else if(m_userType == User_Type::User_Time)             //如果使用者是 时间
    {
        QPen penText;
        penText.setColor(QColor(153,153,153));
        painter.setPen(penText);
        QTextOption option(Qt::AlignCenter);                //等价于 Qt::AlignHCenter | Qt::AlignVCenter，水平垂直居中
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        QFont te_font = this->font();
        te_font.setFamily("MicrosoftYaHei");
        te_font.setPointSize(10);
        painter.setFont(te_font);
        painter.drawText(this->rect(),m_curTime,option);
    }
};
