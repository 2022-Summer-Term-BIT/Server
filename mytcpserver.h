#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpServer>

class myTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit myTcpServer(QObject *parent = nullptr);

signals:
    qintptr newSocketDescriptor(qintptr socket);

protected:
    virtual void incomingConnection(qintptr socketDescriptor);

};

#endif // MYTCPSERVER_H
