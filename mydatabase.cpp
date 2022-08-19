#include "mydatabase.h"
#include <QSqlQuery>

void myDataBase::createConnection()
{
    // 初始化添加数据库类型
    db = QSqlDatabase::addDatabase("QSQLITE");
    //判断是否建立了用户表
    db.setDatabaseName("./people.db");
    db.open();
    QSqlQuery sqlquery;
    sqlquery.exec("CREATE TABLE if not exists people (id INTEGER NOT NULL UNIQUE,name TEXT NOT NULL UNIQUE,password TEXT NOT NULL,ip TEXT,islogin INTEGER NOT NULL,PRIMARY KEY(id AUTOINCREMENT))");
    db.close();
}

void myDataBase::dbOpen()
{
    db.setDatabaseName("./people.db");
    db.open();
}

void myDataBase::dbClose()
{
    db.close();
}

