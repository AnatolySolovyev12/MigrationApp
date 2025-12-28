#include <QtCore/QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <qsqlquery>

void connectDataBase(QSqlDatabase& tempDbConnection, bool masterBool);
void getTablesArray();
QString getDataBaseName(QString paramString);
void createNewDbFromOther(QString tempName);


QList<QString>stringTablesArray;
QSqlDatabase mainDb;
QSqlDatabase masterDb;
QSqlDatabase doppelDb;
QString nameDb;

int main(int argc, char* argv[])
{
	qDebug() << "All drivers for connection: " << QSqlDatabase::drivers() << "\n";

	QCoreApplication app(argc, argv);

	connectDataBase(mainDb, 0);

	//getTablesArray();
	
	connectDataBase(masterDb, 1);

	//createNewDbFromOther(nameDb);

	//qDebug() << stringTablesArray;

	return app.exec();
}



void connectDataBase(QSqlDatabase& tempDbConnection, bool masterBool)
{
	if (!masterBool)
	{
		/*
		mainDb = QSqlDatabase::addDatabase("QPSQL"); // Работаем используя библиотеки (PostgreSQL) и отдельные методы для подключения. Вкладываем их потом в папку с программой.
		mainDb.setHostName("192.168.56.101");
		mainDb.setDatabaseName("testdb");
		mainDb.setUserName("solovev");
		mainDb.setPassword("art54011212");
		mainDb.setPort(5432);  // По умолчанию 5432
		*/

		//QSqlDatabase mainDb = QSqlDatabase::addDatabase("QODBC"); //  Работаем через QODBC в таком варианте для подключения напрямую к PostgreSQL (работает без отдельных библиотек)
        //mainDb.setDatabaseName("DRIVER={PostgreSQL ANSI};SERVER=192.168.56.101;PORT=5432;DATABASE=testdb;UID=solovev;PWD=art54011212");

		tempDbConnection = QSqlDatabase::addDatabase("QODBC", "mainDbConn"); // Работаем используя QODBC (SQL Server) и отдельную строку. Методы не сработают. 
		tempDbConnection.setDatabaseName(
			"DRIVER={SQL Server};"
			"Server=127.0.0.1,1433;"
			"Database=EnergyRes;"
			"Uid=solovev;"
			"Pwd=art54011212;"
		);
	}
	else
	{
		QString connStr = "DRIVER={ODBC Driver 17 for SQL Server};"
			"Server=127.0.0.1,1433;DATABASE=master;Trusted_Connection=yes;"; // Windows Authentication
		tempDbConnection = QSqlDatabase::addDatabase("QODBC", "masterConn");
		tempDbConnection.setDatabaseName(connStr);
	}

	if (!tempDbConnection.open())
	{
		if (tempDbConnection.lastError().isValid())
		{
			qDebug() << tempDbConnection.lastError().text();
			qDebug() << tempDbConnection.lastError().driverText();
			qDebug() << tempDbConnection.lastError().databaseText();
		}
		else
			qDebug() << "Unknow error happened in connectDataBase()";
	}
	else
	{
		nameDb = masterBool == true ? "master with Windows Authentication" : getDataBaseName(tempDbConnection.databaseName());
		qDebug() << "DataBase is CONNECT to " + nameDb + " with connection name = " + tempDbConnection.connectionName()<< '\n';
	}
}



void getTablesArray()
{
	QSqlQuery tablesQuery(mainDb);

	tablesQuery.prepare(R"(
SELECT TABLE_NAME
FROM INFORMATION_SCHEMA.TABLES
WHERE TABLE_SCHEMA = 'dbo' AND TABLE_TYPE = 'BASE TABLE' AND TABLE_CATALOG = :dbNameFromConnect
        )");

	tablesQuery.bindValue(":dbNameFromConnect", nameDb);
	//tablesQuery.addBindValue(nameDb); Альтернатива. Будет добавлен вместо ? в запросе

	if (!tablesQuery.exec() || !tablesQuery.next())
	{
		tablesQuery.lastError().text();
	}
	else
	{
		do {
			stringTablesArray.push_back(tablesQuery.value(0).toString());
		} while (tablesQuery.next());
	}
}



QString getDataBaseName(QString paramString)
{
	QString temp = paramString.sliced(paramString.indexOf("Database=")).sliced(9);
	temp = temp.left(temp.indexOf(';'));

	return temp;
}


void createNewDbFromOther(QString tempName)
{
	QSqlQuery createBase(mainDb);

	QString tempStringForName = tempName + "_doppelganger";
	createBase.prepare(R"(
CREATE DATABASE IF NOT EXIST :dbName
        )");

	createBase.bindValue(":dbName", tempStringForName);

	if (!createBase.exec())
	{
		qDebug() << tempStringForName + "wasnt create because:\n";
		qDebug() << createBase.lastError().text();
		qDebug() << createBase.lastError().databaseText();
		qDebug() << createBase.lastError().driverText();
	}
	else
	{
		qDebug() << tempStringForName + " was create.";
	}

}
