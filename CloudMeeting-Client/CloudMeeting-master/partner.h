#ifndef PARTNER_H
#define PARTNER_H

//房间信息
#include <QLabel>

class Partner : public QLabel       //w178,371,519,590,646,677,706,736
{
    Q_OBJECT
private:
    quint32 ip;

    void mousePressEvent(QMouseEvent *ev) override;
    int w;
public:
    Partner(QWidget * parent = nullptr, quint32 = 0);
    void setpic(QImage img);            //w179,372,554,593
signals:
    void sendip(quint32); //发送ip    w654,678
};

#endif // PARTNER_H
