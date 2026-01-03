#include <QtCore/QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <qsqlquery>
#include <iostream>



bool connectDataBase(QSqlDatabase& tempDbConnection, bool masterBool);
bool getTablesArray();
QString getDataBaseName(QString paramString);
void createDoppelDbFromMain(QString tempName);
void readTablesInMainDb(QString baseName, QString tableNameTemp);
void createTablesInDoppelDb(QString baseName, QString tableNameTemp);



QList<QString>stringTablesArray;

QSqlDatabase mainDb;
QSqlDatabase masterDb;
QSqlDatabase doppelDb;

QString nameDb;

struct TableColumnStruct
{
	QString tableCatalog = "";
	QString tableSchema = "";
	QString tableName = "";
	QString ColumnName = "";
	int ordinalPosition;
	QString columnDefault = "";
	QString isNullable = "";
	QString dataType = "";
	int characterMaximumLength;
	int characterOctetLength;
	int numericPrecision;
	int numericPrecisionRadix;
	int numericScale;
	int dateTimePrecision;
	QString characterSetCatalog = "";
	QString characterSetSchema = "";
	QString characterSetName = "";
	QString collationCatalog = "";
	QString collatiionSchema = "";
	QString collationName = "";
	QString domainCatalog = "";
	QString domainSchena = "";
	QString domainName = "";
};

QList<TableColumnStruct>structArrayForTable;



int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "russian");

	qDebug() << "All drivers for connection: " << QSqlDatabase::drivers() << "\n";

	QCoreApplication app(argc, argv);

	if (connectDataBase(mainDb, 0))
	{
		if (getTablesArray())
		{
			if (connectDataBase(masterDb, 1))
				createDoppelDbFromMain(getDataBaseName(mainDb.databaseName()));
			else
				qDebug() << "Check your setting for connect to master DataBase and try again";
		}
		else
			qDebug() << "Fail when trying to get all tables from mainDb";	
	}
	else
	{
		qDebug() << "Check your setting for connect to main DataBase and try again";
	}

	return app.exec();
}



bool connectDataBase(QSqlDatabase& tempDbConnection, bool masterBool)
{
	// Производим подключение к целевой БД

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
			std::cout << "Error in connectDataBase: " << tempDbConnection.lastError().text().toStdString() << std::endl;
			std::cout << "Error in connectDataBase: " << tempDbConnection.lastError().driverText().toStdString() << std::endl;
			std::cout << "Error in connectDataBase: " << tempDbConnection.lastError().databaseText().toStdString() << std::endl;

			return false;
		}
		else
		{
			qDebug() << "Error in connectDataBase. Unknow error happened in connectDataBase()";
			return false;
		}
	}
	else
	{
		nameDb = masterBool == true ? "master using Windows Authentication" : getDataBaseName(tempDbConnection.databaseName());
		qDebug() << "DataBase is CONNECT to " + nameDb + " with connection name = " + tempDbConnection.connectionName() << '\n';
		return true;
	}
}



bool getTablesArray()
{
	// Получаем список всех таблиц в донорской БД

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
		std::cout << "Error in getTablesArray: " << tablesQuery.lastError().text().toStdString() << std::endl;
		return false;
	}
	else
	{
		do {
			stringTablesArray.push_back(tablesQuery.value(0).toString());
		} while (tablesQuery.next());

		return true;
	}
}



QString getDataBaseName(QString paramString)
{
	// Получаем чисто имя базы данных

	QString temp = paramString.sliced(paramString.indexOf("Database=")).sliced(9);
	temp = temp.left(temp.indexOf(';'));
	return temp;
}



void createDoppelDbFromMain(QString tempName)
{
	// Создаём новую БД с изменённым именем

	QSqlQuery createBase(masterDb);

	QString tempStringForName = tempName + "_doppelganger";

	createBase.prepare(R"(
SELECT name
  FROM [msdb].[sys].[databases]
WHERE name = :tableNameCheck
        )");

	createBase.bindValue(":tableNameCheck", tempStringForName);

	if (!createBase.exec() || !createBase.next())
	{
		if (createBase.lastError().isValid())
		{
			std::cout << "Error in createNewDbFromOther when check new DB: " << createBase.lastError().text().toStdString() << std::endl;
			return;
		}

		QString createDataBaseStringQuery = QString("CREATE DATABASE %1").arg(tempStringForName); // создание БД нельзя в SQL Server провести с использованием placeholders. Подготовленные запросы не пройдут.

		if (!createBase.exec(createDataBaseStringQuery))
		{
			qDebug() << "Error in createNewDbFromOther when create new DB: " << tempStringForName + " wasnt create because:" << "\n";
			std::cout << createBase.lastError().text().toStdString() << std::endl;
			std::cout << createBase.lastError().databaseText().toStdString() << std::endl;
			std::cout << createBase.lastError().driverText().toStdString() << std::endl;
		}
		else
		{
			qDebug() << tempStringForName + " was create" << "\n";
		}
	}
	else
	{
		if (createBase.isValid())
		{
			qDebug() << "DataBase with this name exists. Try use other name or check this DataBase\n";
			return;
		}
	}

	for (auto& nameTableFromCycle : stringTablesArray)
	{
		readTablesInMainDb(tempStringForName, nameTableFromCycle);
	}
}



void readTablesInMainDb(QString baseName, QString tableNameTemp)
{
	// Формируем список столбцов для создаваемой таблице

	QSqlQuery createTableAndColumn(mainDb);

	createTableAndColumn.prepare(R"(
SELECT *
  FROM INFORMATION_SCHEMA.COLUMNS
  where TABLE_NAME = :tempTableName
  order by ORDINAL_POSITION
        )");

	createTableAndColumn.bindValue(":tempTableName", tableNameTemp);

	if (!createTableAndColumn.exec() || !createTableAndColumn.next())
	{
		if (createTableAndColumn.lastError().isValid())
		{
			std::cout << "Error in readTableFuncFromDonorBase when check column for tables of new DB: " << createTableAndColumn.lastError().text().toStdString() << std::endl;
			return;
		}
	}
	else
	{
		do {
			TableColumnStruct tempStruct;

			tempStruct.tableCatalog = createTableAndColumn.value(0).toString();
			tempStruct.tableSchema = createTableAndColumn.value(1).toString();
			tempStruct.tableName = createTableAndColumn.value(2).toString();
			tempStruct.ColumnName = createTableAndColumn.value(3).toString();
			tempStruct.ordinalPosition = createTableAndColumn.value(4).toInt();
			tempStruct.columnDefault = createTableAndColumn.value(5).toString();
			tempStruct.isNullable = createTableAndColumn.value(6).toString();
			tempStruct.dataType = createTableAndColumn.value(7).toString();
			tempStruct.characterMaximumLength = createTableAndColumn.value(8).toInt();
			tempStruct.characterOctetLength = createTableAndColumn.value(9).toInt();
			tempStruct.numericPrecision = createTableAndColumn.value(10).toInt();
			tempStruct.numericPrecisionRadix = createTableAndColumn.value(11).toInt();
			tempStruct.numericScale = createTableAndColumn.value(12).toInt();
			tempStruct.dateTimePrecision = createTableAndColumn.value(13).toInt();
			tempStruct.characterSetCatalog = createTableAndColumn.value(14).toString();
			tempStruct.characterSetSchema = createTableAndColumn.value(15).toString();
			tempStruct.characterSetName = createTableAndColumn.value(16).toString();
			tempStruct.collationCatalog = createTableAndColumn.value(17).toString();
			tempStruct.collatiionSchema = createTableAndColumn.value(18).toString();
			tempStruct.collationName = createTableAndColumn.value(19).toString();
			tempStruct.domainCatalog = createTableAndColumn.value(20).toString();
			tempStruct.domainSchena = createTableAndColumn.value(21).toString();
			tempStruct.domainName = createTableAndColumn.value(22).toString();

			structArrayForTable.push_back(tempStruct);

		} while (createTableAndColumn.next());

		createTablesInDoppelDb(baseName, tableNameTemp);
	}

	qDebug() << "Add table " << tableNameTemp;
	structArrayForTable.clear();
}



void createTablesInDoppelDb(QString baseName, QString tableNameTemp)
{
	// Проверяем наличие дубликатов таблиц в новой БД

	QSqlQuery createTableAndColumnInNewDb(masterDb);

	QString queryString = QString("SELECT * FROM %1.INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = '%2'")
		.arg(baseName)
		.arg(tableNameTemp);

	if (!createTableAndColumnInNewDb.exec(queryString) || !createTableAndColumnInNewDb.next())
	{
		if (createTableAndColumnInNewDb.lastError().isValid())
		{
			std::cout << "Error in createTablesInDoppelDb when trying to find duplicate tables: " << createTableAndColumnInNewDb.lastError().text().toStdString() << std::endl;
			structArrayForTable.clear();
			return;
		}
	}
	else
	{
		if (createTableAndColumnInNewDb.isValid())
		{
			qDebug() << "DataBase have duplicate tables (" << tableNameTemp << "). Try to check doppelganger DB" << "\n";
			structArrayForTable.clear();
			return;
		}
	}

	// Создаём новую таблицу в новой БД

	queryString = QString("CREATE TABLE %1.dbo.%2 (%3 %4 %5)")
		.arg(baseName)
		.arg(tableNameTemp)
		.arg(structArrayForTable[0].ColumnName)
		.arg(structArrayForTable[0].dataType)
		.arg(structArrayForTable[0].isNullable == "YES" ? "" : "NOT NULL");

	if (!createTableAndColumnInNewDb.exec(queryString) || !createTableAndColumnInNewDb.next())
	{
		if (createTableAndColumnInNewDb.lastError().isValid())
		{
			std::cout << "Error in createTablesInDoppelDb when try create new table(" << tableNameTemp.toStdString() << "): " << createTableAndColumnInNewDb.lastError().text().toStdString() << std::endl;
			structArrayForTable.clear();
			return;
		}
	}

	// Добавляем оставшиеся столбцы в созданную таблицу

	for (int counterColumn = 1; counterColumn < structArrayForTable.length(); counterColumn++)
	{
		queryString = QString("ALTER TABLE %1.dbo.%2 ADD %3 %4 %5")
			.arg(baseName)
			.arg(tableNameTemp)
			.arg(structArrayForTable[counterColumn].ColumnName)
			.arg(structArrayForTable[counterColumn].dataType == "varchar" ? ("varchar(" + QString::number(structArrayForTable[counterColumn].characterMaximumLength) + ")") : structArrayForTable[counterColumn].dataType)
			.arg(structArrayForTable[counterColumn].isNullable == "YES" ? "" : "NOT NULL");

		if (!createTableAndColumnInNewDb.exec(queryString))
		{
			if (createTableAndColumnInNewDb.lastError().isValid())
			{
				std::cout << "Error in createTablesInDoppelDb when try add new column: " << createTableAndColumnInNewDb.lastError().text().toStdString() << std::endl;
				structArrayForTable.clear();
				return;
			}
		}
	}
}
