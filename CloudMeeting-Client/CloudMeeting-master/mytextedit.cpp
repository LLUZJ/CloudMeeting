//文本编辑控件

#include "mytextedit.h"
#include <QVBoxLayout>                  //垂直布局
#include <QStringListModel>             //处理字符串列表的数据模型
#include <QDebug>
#include <QAbstractItemView>            //提供了项目视图类的基本功能
#include <QScrollBar>                   //提供垂直或水平滚动条

Completer::Completer(QWidget *parent): QCompleter(parent)
{

}

MyTextEdit::MyTextEdit(QWidget *parent): QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);        //布局
    layout->setContentsMargins(0, 0, 0, 0);             //设置左侧、顶部、右侧和底部边距，以便在布局周围使用
    edit = new QPlainTextEdit();
    edit->setPlaceholderText(QString::fromUtf8("&#x8F93;&#x5165;@&#x53EF;&#x4EE5;&#x5411;&#x5BF9;&#x5E94;&#x7684;IP&#x53D1;&#x9001;&#x6570;&#x636E;"));     //QTextEdit 和 QLineEdit 都有 placeholderText 属性，即在输入内容之前，给予用户一些提示信息
    layout->addWidget(edit);                            //往layout布局中添加edit控件
    completer = nullptr;
    connect(edit, SIGNAL(textChanged()), this, SLOT(complete()));       //当edit编辑控件发出textChanged文本改变信号，文本编辑类调用complete
    edit->installEventFilter(this);                     //给edit文本编辑控件安装一个过滤器 即文本编辑类
}

QString MyTextEdit::textUnderCursor()                   //光标标下的文本
{
    QTextCursor tc = edit->textCursor();                //在edit文本编辑器中创建文本光标对象tc；  文档可以通过 QTextCursor 类提供的接口进行编辑;游标是使用构造函数创建的，也可以是从编辑器小部件获取的
    tc.select(QTextCursor::WordUnderCursor);            //选择光标下的文本
    return tc.selectedText();                           //返回选择的文本
}

void MyTextEdit::complete()                         //自动补全  23
{
    if(edit->toPlainText().size() == 0 || completer == nullptr) return;     //edit编辑控件中toPlainText：多行简单文本框的大小=0，且completer自动补全对象为空，就退出

    QChar tail =  edit->toPlainText().at(edit->toPlainText().size()-1);     //给字符处理对象tail赋值 edit文本编辑器的文本框.at去获取文本框大小-1的文本内容    QChar处理字符的基本类型
    if(tail == '@')                 //如果tail为字符@
    {
        completer->setCompletionPrefix(tail);                               //setCompletionPrefix设置补全提示的前缀为 tail此时为@
        QAbstractItemView *view = completer->popup();                       //创建项目视图view，创建一个popup弹出框，用来显示临时的，模态或非模态的弹出窗口
        view->setCurrentIndex(completer->completionModel()->index(0, 0));   //view设置当前页面索引
        QRect cr = edit->cursorRect();                                      //创建一个文本编辑器光标矩形cr  QRect:使用整数精度在平面上定义了一个矩形
        QScrollBar *bar = completer->popup()->verticalScrollBar();          //创建水平滑块bar在popup弹出框上
        cr.setWidth(completer->popup()->sizeHintForColumn(0) + bar->sizeHint().width());    //给光标矩形cr设置宽度
        completer->complete(cr);                                            //把光标矩形cr加到自动补全中
    }
}

void MyTextEdit::changeCompletion(QString text)                         //更改自动补全    107
{
    QTextCursor tc = edit->textCursor();                                //创建一个文本光标tc
    int len = text.size() - completer->completionPrefix().size();       //len的大小为 文本大小 - 自动补全 的前缀大小
    tc.movePosition(QTextCursor::EndOfWord);                            //文本光标tc 移动位置 到 单词末尾
    tc.insertText(text.right(len));                                     //tc插入文本 （文本正确 的长度）
    edit->setTextCursor(tc);                                            //edit文本编辑器 设置文本光标为 文本光标tc
    completer->popup()->hide();                                         //隐藏 弹出框popup

    QString str = edit->toPlainText();                                  //把文本编辑器的纯文本 赋给str
    int pos = str.size() - 1;                                           //pos = str的大小 -1
    while(str.at(pos) != '@') pos--;                                    //str 查询pos的位置 如果不是 @ ；则pos--

    tc.clearSelection();                                                //清除tc中选择
    tc.setPosition(pos, QTextCursor::MoveAnchor);                       //tc 设置MoveAnchor移动锚的位置 在文本的第pos位置
    tc.setPosition(str.size(), QTextCursor::KeepAnchor);                //tc 设置KeepAnchor运动锚的位置 在文本的第str.size位置
      // tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, str.size() - pos);

    QTextCharFormat fmt = tc.charFormat();                              //QTextCharFormat：设置文本编辑框字体，字号，加粗，倾斜，下划线，颜色
    QTextCharFormat fmt_back = fmt;
    fmt.setForeground(QBrush(Qt::white));                               //设置编辑框前景 用画刷设为白色
    fmt.setBackground(QBrush(QColor(0, 160, 233)));                     //设置编辑框背景 用画刷设为x色
    tc.setCharFormat(fmt);                                              //文本光标tc 设置字符格式 为fmt
    tc.clearSelection();                                                //清除文本光标tc的选择
    tc.setCharFormat(fmt_back);                                         //文本光标tc 设置字符格式 为fmt_back

    tc.insertText(" ");                                                 //tc 插入文本 “空格”
    edit->setTextCursor(tc);                                            //文本编辑器edit 设置文本光标 为tc

    ipspan.push_back(QPair<int, int>(pos, str.size()+1));               //把(pos, str.size()+1)添加到 vector容器ipspan 的队尾

}

QString MyTextEdit::toPlainText()           //装换为纯文本    w786
{
    return edit->toPlainText();             //返回纯文本内容
}

void MyTextEdit::setPlainText(QString str)  //设置纯文本     w795
{
    edit->setPlainText(str);                //文本编辑器edit 设置纯文本内容 str
}

void MyTextEdit::setPlaceholderText(QString str)        //设置设置占位符文本
{
    edit->setPlaceholderText(str);          //文本编辑器edit 设置占位符文本 为str
}

void MyTextEdit::setCompleter(QStringList stringlist)       //设置自动补全    w311,557,569,597,620
{
    if(completer == nullptr)
    {
        completer = new Completer(this);
        completer->setWidget(this);
        completer->setCompletionMode(QCompleter::PopupCompletion);          //设置自动补全模式 ：弹出列表,但只会出现匹配的ITEM
        completer->setCaseSensitivity(Qt::CaseInsensitive);                 //设置大小写敏感性 ：不区分大小写
        connect(completer, SIGNAL(activated(QString)), this, SLOT(changeCompletion(QString)));      //当自动补全对象发出activated(单击下拉框)信号，文本编辑类调用changeCompletion更改自动补全
    }
    else
    {
        delete completer->model();                                          //删除自动补全对象的模式
    }
    QStringListModel * model = new QStringListModel(stringlist, this);
    completer->setModel(model);
}

bool MyTextEdit::eventFilter(QObject *obj, QEvent *event)                   //重写事件过滤器
{
    if(obj == edit)     //如果obj 是 edit文本编辑器
    {
        if(event->type() == QEvent::KeyPress)           //如果事件类型 为 按键事件
        {
            QKeyEvent *keyevent = static_cast<QKeyEvent *>(event);          //创建一个鼠标事件 keyevent
                QTextCursor tc = edit->textCursor();                        //tc 为edit文本编辑器的文本光标
                int p = tc.position();                                      //p 是tc的位置
                int i;
                for(i = 0; i < ipspan.size(); i++)                          //ipspan是vector容器
                {
                    //如果鼠标事件的键是 空格键 且p > ipspan中i的key值 且 p <= ipspan中i的value值   或者按键事件的键 为 清除键 且 p >= ipspan中i的key值，且 p<ipspan中i的value值
                    if( (keyevent->key() == Qt::Key_Backspace && p > ipspan[i].first && p <= ipspan[i].second ) || (keyevent->key() == Qt::Key_Delete && p >= ipspan[i].first && p < ipspan[i].second) )
                    {
                        tc.setPosition(ipspan[i].first, QTextCursor::MoveAnchor);           //tc设置 MoveAnchor移动锚 的位置 在ipspan中i的key值位置
                        if(p == ipspan[i].second)                                           //如果p = 容器的value值
                        {
                            tc.setPosition(ipspan[i].second, QTextCursor::KeepAnchor);      //tc设置 KeepAnchor运动锚 的位置 在ipspan中i的value值位置
                        }
                        else
                        {
                            tc.setPosition(ipspan[i].second + 1, QTextCursor::KeepAnchor);  //否则 tc设置 KeepAnchor运动锚 的位置 在ipspan中i的value值 + 1 的位置
                        }
                        tc.removeSelectedText();                                            //tc 移除选择文本
                        ipspan.removeAt(i);                                                 //容器  移除i
                        return true;
                    }
                    else if(p >= ipspan[i].first && p <= ipspan[i].second)                  //如果p >= 容器的key 且 p<= 容器的value值
                    {
                        QTextCursor tc = edit->textCursor();
                        tc.setPosition(ipspan[i].second);                                   //tc 设置位置在 容器i的value值
                        edit->setTextCursor(tc);                                            //文本编辑器edit 设置文本光标 为tc
                    }
                }
        }
    }
    return QWidget::eventFilter(obj, event);                                                //返回事件过滤器
}
