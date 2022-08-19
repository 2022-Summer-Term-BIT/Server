#ifndef MYDATABASE_H
#define MYDATABASE_H

#include <QSqlDatabase>

class myDataBase : public QSqlDatabase
{

public:

    QSqlDatabase db;

    void createConnection();                                        //创建一个数据库连接
    void dbOpen();                                                  //打开数据库
    void dbClose();                                                 //关闭数据库

signals:

private:

};

#endif // MYDATABASE_H
