#include <QtCore/QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>


void connectDataBase();


int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	connectDataBase();

	return app.exec();
}


void connectDataBase()
{
	QSqlDatabase mainDb = QSqlDatabase::addDatabase("QPSQL"); // Работаем используя библиотеки PostgreSQL
	mainDb.setHostName("192.168.56.101");
	mainDb.setDatabaseName("testdb");
	mainDb.setUserName("solovev");
	mainDb.setPassword("art5401121");
	mainDb.setPort(5432);  // По умолчанию 5432

	//QSqlDatabase mainDb = QSqlDatabase::addDatabase("QODBC"); //  Работаем через ODBC
	//mainDb.setDatabaseName("DRIVER={PostgreSQL ANSI};SERVER=192.168.56.101;PORT=5432;DATABASE=testdb;UID=solovev;PWD=art54011212");

	if (!mainDb.open())
	{
		if (mainDb.lastError().isValid())
		{
			qDebug() << mainDb.lastError().text();
			qDebug() << mainDb.lastError().driverText();
			qDebug() << mainDb.lastError().databaseText();
		}
		else
			qDebug() << "Unknow error happened in connectDataBase()";
	}
	else
	{
		qDebug() << "DataBase is CONNECT.";
	}
}