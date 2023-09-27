#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

//文本编辑控件
#include <QWidget>
#include <QPlainTextEdit>           //多行纯文本编辑器控件，用于显示和编辑多行简单文本
#include <QCompleter>               //能实现自动填充功能,方便用户输入,提升用户的体验,一般和QLineEdit与QComboBox搭配起来使用
#include <QStringList>              //QStringList是存储QString的QList.所以你可以认为QStringList是存储类型为QString的列表
#include <QPair>                    //pair将一对值（可以是不同的数据类型）组合成一个值，两个值可以分别用pair的两个公有函数first和second访问
#include <QVector>                  //主要用于数据存储，可以类比C++中的泛型容器vector（动态数组）

class Completer: public QCompleter
{
Q_OBJECT
public:
    explicit Completer(QWidget *parent= nullptr);
};

class MyTextEdit : public QWidget
{
    Q_OBJECT
private:
    QPlainTextEdit *edit;
    Completer *completer;
    QVector<QPair<int, int> > ipspan;
public:
    explicit MyTextEdit(QWidget *parent = nullptr);
    QString toPlainText();                          //w786
    void setPlainText(QString);                     //w795
    void setPlaceholderText(QString);
    void setCompleter(QStringList );                //w311,557,569,597,620
private:
    QString textUnderCursor();
    bool eventFilter(QObject *, QEvent *);

private slots:
    void changeCompletion(QString);
public slots:

    void complete();
signals:
};

#endif // MYTEXTEDIT_H
