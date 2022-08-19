#include "mythread.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>

QSqlQuery logQuery;

myThread::myThread(qintptr socket, QObject *parent)
    : QThread{parent}
{
    m_tcpSocket = new QTcpSocket(this);
    m_tcpSocket->setSocketDescriptor(socket);
}



void myThread::run()
{

    // 处理客户端请求
    connect(m_tcpSocket,&QTcpSocket::readyRead,this,[=]()
    {
        // 创建数据库连接
        m_DB.createConnection();

        // 获取服务器ip地址和端口
        ip = m_tcpSocket->peerAddress().toString();
        port = m_tcpSocket->peerPort();
        QString str = QString("[IP:%1,Port:%2]").arg(ip).arg(port);
        qDebug() << str ;

        //读取缓冲区数据，获取用户请求
        QByteArray buffer = m_tcpSocket->readAll();
        QString request = QString(buffer);
        QString requestTpye = request.section("##",0,0);

        // 判断用户请求类型
        // ①登录时客户端发送                   login##name##password 类型的信号
        // ②注册时客户端发送                   register##name##psw
        // ③想要私聊时客户端发送                chat_send ## 时间 ## 发送者 ## 接收者 ## msg
        // ④想要退出登录时客户端发送            logout ##
        // ⑤获取好友列表及在线状态、未读信息等    getfriendlist ## id ##
        // ⑥请求添加好友                      add_friend ## whowantadd_id ## friend_name
        // ⑦请求删除好友                      delete_friend ## whowantdelete_id ## friend_name
        // ⑧发送文件
        if(requestTpye == "login")
        {
            loginRequsetProcess(request);
        }
        if(requestTpye == "register")
        {
            registerRequsetProcess(request);
        }
        if(requestTpye == "chat_send")
        {
            chatBeginRequestProcess(request);
        }
        if(requestTpye == "logout")
        {
            logoutRequestProcess(request);
        }
        if(requestTpye == "getfriendlist")
        {
            getFriendListRequestProcess(request);
        }
        if(requestTpye == "add_friend")
        {
            addFriendRequestProcess(request);
        }
        if(requestTpye == "delete_friend")
        {
            deleteFriendRequestProcess(request);
        }

        // 退出程序时，释放套接字 close+deletelater， 随后发射结束信号over
    });

    // 进入消息循环
    this->exec();
}

void myThread::loginRequsetProcess(QString req)
{
    //打开数据库，查询有无name用户
    m_DB.dbOpen();

    QSqlQuery sqlquery;
    sqlquery.prepare("select * from people where name = :name");
    sqlquery.bindValue(":name",req.section("##",1,1));
    sqlquery.exec();
    if(!sqlquery.next())
    {
        //未查找到该用户
        m_tcpSocket->write(QString("login error##no_user").toUtf8());
        // ?
        m_tcpSocket->flush();
        m_DB.dbClose();
    }
    else
    {
        //用户存在 查询到id name password ip islogin
        int id = sqlquery.value(0).toInt();
        QString pwd = sqlquery.value(2).toString();
        // 将查询的psw 与 登陆时发送的psw对比，相同则登陆成功
        if(pwd == req.section("##",2,2))
        {
            //登录成功
            m_tcpSocket->write(QString("login successed##%1").arg(id).toUtf8());
            sqlquery.prepare("update people set ip=:ip, islogin=1 where name = :name");
            sqlquery.bindValue(":ip",ip);
            sqlquery.bindValue(":name",req.section("##",1,1));
            sqlquery.exec();
            m_tcpSocket->flush();
            logQuery = sqlquery;
            //更新服务器界面  发射信号至服务器cpp处理
            emit logUpdateSignal();
        }
        else
        {
            //密码错误

            m_tcpSocket->write(QString("login error##errpwd").toUtf8());
            m_tcpSocket->flush();
            m_DB.dbClose();
        }
    }
}

void myThread::registerRequsetProcess(QString req)
{
    // 打开数据库
    m_DB.dbOpen();

    QSqlQuery sqlquery;
    // 注册用户的时候需要进行判重
    qDebug() << "1";
    sqlquery.prepare("select * from people where name = :name");
    sqlquery.bindValue(":name",req.section("##",1,1));
    sqlquery.exec();
    qDebug() << "2";
    if(!sqlquery.next())
    {
        //可以新建
        sqlquery.clear();
        sqlquery.prepare("insert into people values (null,:name,:password,null,0)");
        sqlquery.bindValue(":name",req.section("##",1,1));
        sqlquery.bindValue(":password",req.section("##",2,2));
        sqlquery.exec();
        qDebug() << "3";
        sqlquery.clear();
        sqlquery.prepare("select * from people where name = :name");
        sqlquery.bindValue(":name",req.section("##",1,1));
        sqlquery.exec();
        sqlquery.next();
        qDebug() << "4";
        // 这里新用户id生成，autoincrement
        int newid = sqlquery.value(0).toInt();

        // 创建新用户的好友列表信息库 sendmessage int 代表好友给新用户发送的未读信息标志| sendfile int 同上
        sqlquery.exec("create table if not exists friend__" + QString::number(newid) +"(id INTEGER unique, name TEXT,sendmassage INTEGER,sendfile INTEGER)");
        qDebug() << "5";
        m_tcpSocket->write(QString("register successed").toUtf8());
        m_tcpSocket->flush();
        m_DB.close();
    }
    else
    {
        // 有重名
         m_tcpSocket->write(QString("register error##same_name").toUtf8());
         m_tcpSocket->flush();
         m_DB.dbClose();
    }
}

void myThread::chatBeginRequestProcess(QString req)
{
    // 打开数据库
    m_DB.dbOpen();

    // chat_send ## 时间time ## 发送者id one ## 接收者id two ## msg
    QDateTime nowstr = QDateTime::fromString(req.section("##",1,1), "yyyy-MM-dd hh:mm:ss.zzz");
    int idone = req.section("##",2,2).toInt();
    int idtwo = req.section("##",3,3).toInt();

    QSqlQuery sqlquery;
    QString sqlstring = "";
    if(idone < idtwo)
    {
        sqlstring = "insert into chat__" + QString::number(idone) + "__" + QString::number(idtwo) + " values(:time,:id,:message)";
    }
    else
    {
        sqlstring = "insert into chat__" + QString::number(idtwo) + "__" + QString::number(idone) + " values(:time,:id,:message)";
    }
    qDebug()<<sqlstring;

    // 更新聊天记录库
    sqlquery.prepare(sqlstring);
    sqlquery.bindValue(":time",nowstr);
    sqlquery.bindValue(":id",idone);
    sqlquery.bindValue(":message",req.section("##",4,4));
    sqlquery.exec();

    // 更新好友消息库
    sqlstring = "update friend__" + QString::number(idtwo) + " set sendmassage = 1 where id = :id";
    m_DB.dbOpen();
    // 在此打开数据库应该是没必要的
    sqlquery.clear();
    sqlquery.prepare(sqlstring);
    sqlquery.bindValue(":id", idone);
    sqlquery.exec();
    m_DB.dbClose();
}

void myThread::logoutRequestProcess(QString req)
{
    // 打开数据库

    m_DB.dbOpen();

    QSqlQuery sqlquery;
    sqlquery.prepare("update people set islogin=0 where id = :id");
    sqlquery.bindValue(":id",req.section("##",1,1));
    sqlquery.exec();

    logQuery = sqlquery;
    //更新服务器界面 发射信号至服务器cpp处理
    emit logUpdateSignal();
}

void myThread::getFriendListRequestProcess(QString req)
{
    // 打开数据库
    m_DB.dbOpen();

    // 查找好友
    QSqlQuery sqlquery;
    sqlquery.exec("select * from friend__" + req.section("##",1,1) + " desc");

    if(sqlquery.next())
    {
        QList <QString> friendlist;
        QList <QString> friendsendfilelist;
        QList <QString> friendsendmassagelist;
        friendlist.append(sqlquery.value(1).toString());

        int sendmassagenum = 0;
        if(sqlquery.value(2).toString() == '1')
        {
            sendmassagenum++;
        }
        friendsendmassagelist.append(sqlquery.value(2).toString());
        int sendfilenum = 0;
        if(sqlquery.value(3).toString() == '1')
        {
            sendfilenum++;
        }
        friendsendfilelist.append(sqlquery.value(3).toString());

        QString friends = "";
        while(sqlquery.next())
        {
            friendlist.append(sqlquery.value(1).toString());
            if(sqlquery.value(2).toString() == '1')
            {
                sendmassagenum++;
            }
            friendsendmassagelist.append(sqlquery.value(2).toString());
            if(sqlquery.value(3).toString() == '1')
            {
                sendfilenum++;
            }
            friendsendfilelist.append(sqlquery.value(3).toString());
        }

        int onlinefriendnum = 0;
        for( int i = 0; i < friendlist.length(); i++)
        {
            sqlquery.prepare("select * from people where name = :name");
            sqlquery.bindValue(":name",friendlist.at(i));
            sqlquery.exec();
            sqlquery.next();
            QString peopleip = sqlquery.value(3).toString();
            if(sqlquery.value(4).toInt() == 1)
            {
                onlinefriendnum++;                  // 1 在线
                friends = "##" + friendlist.at(i) + "##1##" + peopleip +"##"+friendsendmassagelist.at(i)+"##"+ friendsendfilelist.at(i)+ friends;
            }
            else
            {                                       // 0 不在线
                friends = "##" + friendlist.at(i) + "##0##" + peopleip +"##"+friendsendmassagelist.at(i)+"##"+ friendsendfilelist.at(i)+ friends;
            }
        }
        friends = "getfriendlist_ok##" + QString::number(friendlist.length()) +"##"+QString::number(onlinefriendnum)+"##"+QString::number(sendmassagenum)+"##"+QString::number(sendfilenum)+ friends;
        qDebug()<<friends;
        m_tcpSocket->write(friends.toUtf8());
        m_tcpSocket->flush();
        // 关闭数据库
        m_DB.dbClose();
    }
    else
    {
        //没朋友哟
        m_tcpSocket->write(QString("getfriendlist_error").toUtf8());
        m_tcpSocket->flush();
        // 关闭数据库
        m_DB.dbClose();
    }
}

void myThread::addFriendRequestProcess(QString req)
{
    // 打开数据库
    m_DB.dbOpen();
    // 谁要添加好友
    int whowantadd_id = req.section("##",1,1).toInt();
    // 要添加的好友 昵称为
    QString friend_name = req.section("##",2,2);
    qDebug() << whowantadd_id << friend_name;

    QSqlQuery sqlquery;
    sqlquery.prepare("select * from people where name = :name");
    sqlquery.bindValue(":name",friend_name);
    sqlquery.exec();
    if(sqlquery.next())
    {
        int friend_id = sqlquery.value(0).toInt();
        qDebug() <<friend_id;
        sqlquery.clear();
        QString sqlstring = "insert into friend__" + QString::number(whowantadd_id) + " values(:id,:name,0,0)";
        qDebug()<<sqlstring;
        sqlquery.prepare(sqlstring);
        sqlquery.bindValue(":id",friend_id);
        sqlquery.bindValue(":name",friend_name);
        sqlquery.exec();
        qDebug()<<sqlquery.lastError();
        m_tcpSocket->write(QString("add_friend_ok").toUtf8());
        m_tcpSocket->flush();
        // 关闭数据库
        m_DB.dbClose();
    }
    else
    {
        //找不到该用户
        qDebug() <<"查无此人";
        m_tcpSocket->write(QString("add_friend_error").toUtf8());
        m_tcpSocket->flush();
        m_DB.close();
    }
}

void myThread::deleteFriendRequestProcess(QString req)
{
    // delete_friend ## whowantdelete_id ## friend_name
    // 打开数据库
    m_DB.dbOpen();
    int whowantdelete_id = req.section("##",1,1).toInt();
    QString friend_name = req.section("##",2,2);
    QSqlQuery sqlquery;
    sqlquery.prepare("delete from friend__"+QString::number(whowantdelete_id)+" where name = :name");
    sqlquery.bindValue(":name",friend_name);
    if(sqlquery.exec())
    {
        qDebug() << "在这里";
        m_tcpSocket->write(QString("delete_friend_ok").toUtf8());
        m_tcpSocket->flush();
    }
    else
    {
        m_tcpSocket->write(QString("delete_friend_error").toUtf8());
        m_tcpSocket->flush();
    }
    m_DB.dbClose();
}


