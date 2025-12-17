#include <QtCore/QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <qsqlquery>

void connectDataBase();
void getTablesArray();
QString getDataBaseName(QString paramString);


QList<QString>stringTablesArray;
QSqlDatabase mainDb;
QString nameDb;

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	connectDataBase();

	getTablesArray();

	qDebug() << stringTablesArray;

	return app.exec();
}


void connectDataBase()
{
	/*
	mainDb = QSqlDatabase::addDatabase("QPSQL"); // Работаем используя библиотеки (PostgreSQL) и отдельные методы для подключения. Вкладываем их потом в папку с программой.
	mainDb.setHostName("192.168.56.101");
	mainDb.setDatabaseName("testdb");
	mainDb.setUserName("solovev");
	mainDb.setPassword("art54011212");
	mainDb.setPort(5432);  // По умолчанию 5432
	*/

	QSqlDatabase mainDb = QSqlDatabase::addDatabase("QODBC"); // Работаем используя QODBC (SQL Server) и отдельную строку. Методы не сработают. 
	mainDb.setDatabaseName(
		"DRIVER={SQL Server};"
		"Server=127.0.0.1,1433;"
		"Database=EnergyRes;"
		"Uid=solovev;"
		"Pwd=art54011212;"
	); //"DataBase is CONNECT to DRIVER={SQL Server};Server=127.0.0.1,1433;Database=testdb;Uid=solovev;Pwd=art54011212;"

	//QSqlDatabase mainDb = QSqlDatabase::addDatabase("QODBC"); //  Работаем через QODBC в таком варианте для подключения напрямую к PostgreSQL (работает без отдельных библиотек)
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
		nameDb = getDataBaseName(mainDb.databaseName());
		qDebug() << "DataBase is CONNECT to " + nameDb << '\n';
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

