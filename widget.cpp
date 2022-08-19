#include "widget.h"
#include "ui_widget.h"

#include <QMessageBox>
#include <QSqlQuery>

extern QSqlQuery logQuery;


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 创建QTcpServer 监听套接字对象
    m_tcpServer = new myTcpServer(this);
    // 设置监听
    m_tcpServer->listen(QHostAddress::Any,8888);

    ui->textBrowser->clear();
    ui->textBrowser->insertPlainText("当前无用户在线");

    // 有新的客户端连接请求
    connect(m_tcpServer,&myTcpServer::newSocketDescriptor,this,&Widget::newClientConnectionRequest);
}

Widget::~Widget()
{
    delete ui;
    m_tcpServer->close();
    m_tcpServer->deleteLater();
}

// 响应客户端连接请求
void Widget::newClientConnectionRequest(qintptr socketDescriptor)
{
    // 创建线程对象
    myThread *  m_thread = new myThread(socketDescriptor);
    // 子线程开始运作
    m_thread->start();

    // 释放子线程对象
    connect(m_thread,&myThread::over,this,[=]()
    {
        m_thread->exit();
        m_thread->wait();
        m_thread->deleteLater();
        QMessageBox::information(this,"退出（子线程释放）","退出完成");
    });

    // 线程中发出的日志更新信号与其处理槽连接
    connect(m_thread,&myThread::logUpdateSignal,this,&Widget::logUpdateSlot);
}

// 更新服务器日志
void Widget::logUpdateSlot()
{
    //更新服务器界面
    ui->textBrowser->clear();
    logQuery.prepare("select * from people where islogin = 1");
    logQuery.exec();
    if(logQuery.next())
    {
        QString userid = logQuery.value(0).toString();
        QString username = logQuery.value(1).toString();
        QString userip = logQuery.value(3).toString();

        ui->textBrowser->append("用户ID："+userid+",用户昵称:"+username+",用户IP:"+userip);
        int rownum = 1;
        while (logQuery.next())
        {
            QString userid = logQuery.value(0).toString();
            QString username = logQuery.value(1).toString();
            QString userip = logQuery.value(3).toString();
            ui->textBrowser->append("用户ID："+userid+",用户昵称:"+username+",用户IP:"+userip);
            rownum++;
        }
    }
    else
    {
        ui->textBrowser->clear();
        ui->textBrowser->append("当前无在线用户");
    }
}

