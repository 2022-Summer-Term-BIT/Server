#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "mytcpserver.h"
#include "mythread.h"
#include "mydatabase.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE



class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;
    myTcpServer * m_tcpServer;

private slots:
    void newClientConnectionRequest(qintptr socketDescriptor);
    void logUpdateSlot();

};
#endif // WIDGET_H
