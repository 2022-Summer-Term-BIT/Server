#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include "mydatabase.h"



class myThread : public QThread
{
    Q_OBJECT
public:
    explicit myThread(qintptr socket,QObject *parent = nullptr);

    void loginRequsetProcess(QString req);
    void registerRequsetProcess(QString req);
    void sendMsgRequestProcess(QString req);
    void chatBeginRequestProcess(QString req);
    void logoutRequestProcess(QString req);
    void getFriendListRequestProcess(QString req);
    void addFriendRequestProcess(QString req);
    void deleteFriendRequestProcess(QString req);

signals:
    void over();
    void logUpdateSignal();

protected:
    void run() override;

private:
    QTcpSocket * m_tcpSocket;
    QString ip;
    int port;
    myDataBase m_DB;
};

#endif // MYTHREAD_H
