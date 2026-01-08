#include <QtCore/QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <qsqlquery>
#include <iostream>



bool connectDataBase(QSqlDatabase& tempDbConnection, bool masterBool, bool doppelBool);
bool getTablesArray();
QString getDataBaseName(QString paramString);
bool createDoppelDbFromMain(QString tempName);
void readTablesInMainDb(QString baseName, QString tableNameTemp);
void createTablesInDoppelDb(QString baseName, QString tableNameTemp);
void dropDataBase(QString baseName);
void createView(QString baseName);
void createLogin();
void createUser();
void dropRole();



QList<QString>stringTablesArray;

QSqlDatabase mainDb;
QSqlDatabase masterDb;
QSqlDatabase doppelDb;

QString mainDbName;
QString masterDbName;
QString doppelDbName;

int counterForPercent = 1;
bool checkDuplicateTableBool = false;
bool createFromCopy = false;
bool CopyWithData = false;

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

	//checkDuplicateTableBool = true; /////////////////////////
	//createFromCopy = true; /////////////////////////////

	if (connectDataBase(mainDb, 0, 0))
	{
		if (getTablesArray())
		{
			if (connectDataBase(masterDb, 1, 0))
			{
				if (createDoppelDbFromMain(getDataBaseName(mainDb.databaseName())))
				{
					if (connectDataBase(doppelDb, 0, 1))
					{
						createView(mainDbName);
						createLogin();
						createUser();
					}
					else
						qDebug() << "Check your setting for connect to doppelDb DataBase and try again";
				}
			}
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

	dropDataBase(doppelDbName); ///////////////////

	return app.exec();
}



bool connectDataBase(QSqlDatabase& tempDbConnection, bool masterBool, bool doppelBool)
{
	// Производим подключение к целевой БД

	if (!masterBool)
	{
		if (!doppelBool)
		{
			std::string typeOfDb;
			std::cout << "What type of DataBase will connect now? (S/s - SQL Server / P/p - PostgreSQL)" << std::endl;

			do {
				std::cin >> typeOfDb;
				if (typeOfDb == "S" || typeOfDb == "s" || typeOfDb == "P" || typeOfDb == "p") break;
				qDebug() << "Incorrect symbol. Try again";
			} while (true);

			if (typeOfDb == "S" || typeOfDb == "s")
			{
				tempDbConnection = QSqlDatabase::addDatabase("QODBC", "mainDbConn"); // Работаем используя QODBC (SQL Server) и отдельную строку. Методы не сработают. 
				tempDbConnection.setDatabaseName(
					"DRIVER={ODBC Driver 17 for SQL Server};" // "DRIVER={SQL Server};" - алтернативный вариант написания c помощью старого драйвера (не рекомендуется)
					"Server=127.0.0.1,1433;"
					"Database=EnergyRes;"
					"Uid=solovev;"
					"Pwd=art54011212;"
				);
			}
			else
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
			}
		}
		else
		{
			std::string typeOfDb;
			std::cout << "What type of DataBase will connect now? (S/s - SQL Server / P/p - PostgreSQL)" << std::endl;

			do {
				std::cin >> typeOfDb;
				if (typeOfDb == "S" || typeOfDb == "s" || typeOfDb == "P" || typeOfDb == "p") break;
				qDebug() << "Incorrect symbol. Try again";
			} while (true);

			if (typeOfDb == "S" || typeOfDb == "s")
			{
				tempDbConnection = QSqlDatabase::addDatabase("QODBC", "doppelConn");
				QString connStr = QString("DRIVER={ODBC Driver 17 for SQL Server};"
					"Server=127.0.0.1,1433;DATABASE=%1;Trusted_Connection=yes;")
					.arg(doppelDbName); // Windows Authentication
				tempDbConnection.setDatabaseName(connStr);
			}
		}
	}
	else
	{
		std::string typeOfDb;
		std::cout << "What type of DataBase will connect now? (S/s - SQL Server / P/p - PostgreSQL)" << std::endl;

		do {
			std::cin >> typeOfDb;
			if (typeOfDb == "S" || typeOfDb == "s" || typeOfDb == "P" || typeOfDb == "p") break;
			qDebug() << "Incorrect symbol. Try again";
		} while (true);

		if (typeOfDb == "S" || typeOfDb == "s")
		{
			tempDbConnection = QSqlDatabase::addDatabase("QODBC", "masterConn");
			QString connStr = "DRIVER={ODBC Driver 17 for SQL Server};"
				"Server=127.0.0.1,1433;DATABASE=master;Trusted_Connection=yes;"; // Windows Authentication
			tempDbConnection.setDatabaseName(connStr);
		}
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
		QString tempName = masterBool == true ? "master using Windows Authentication" : getDataBaseName(tempDbConnection.databaseName());
		qDebug() << "DataBase is CONNECT to " + (doppelBool == true ? doppelDbName : tempName) + " with connection name = " + tempDbConnection.connectionName() << '\n';

		if (masterBool)
			masterDbName = "master";
		if (!masterBool && !doppelBool)
			mainDbName = getDataBaseName(tempDbConnection.databaseName());

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

	tablesQuery.bindValue(":dbNameFromConnect", mainDbName);
	//tablesQuery.addBindValue(mainDbName); Альтернатива. Будет добавлен вместо ? в запросе

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



bool createDoppelDbFromMain(QString tempName)
{
	// Создаём новую БД с изменённым именем

	QSqlQuery createBase(masterDb);

	doppelDbName = tempName + "_doppelganger";

	createBase.prepare(R"(
SELECT name
  FROM [msdb].[sys].[databases]
WHERE name = :tableNameCheck
        )");

	createBase.bindValue(":tableNameCheck", doppelDbName);

	if (!createBase.exec() || !createBase.next())
	{
		if (createBase.lastError().isValid())
		{
			std::cout << "Error in createNewDbFromOther when check new DB: " << createBase.lastError().text().toStdString() << std::endl;
			return false;
		}

		QString createDataBaseStringQuery = QString("CREATE DATABASE %1").arg(doppelDbName); // создание БД нельзя в SQL Server провести с использованием placeholders. Подготовленные запросы не пройдут.

		if (!createBase.exec(createDataBaseStringQuery))
		{
			qDebug() << "Error in createNewDbFromOther when create new DB: " << doppelDbName + " wasnt create because:" << "\n";
			std::cout << createBase.lastError().text().toStdString() << std::endl;
			std::cout << createBase.lastError().databaseText().toStdString() << std::endl;
			std::cout << createBase.lastError().driverText().toStdString() << std::endl;

			return false;
		}
		else
		{
			qDebug() << doppelDbName + " was create" << "\n";
		}
	}
	else
	{
		if (createBase.isValid())
		{
			qDebug() << "DataBase with" << doppelDbName << "name exist. Try use other name or check this DataBase\n";
			return false;
		}
	}

	for (auto& nameTableFromCycle : stringTablesArray)
	{
		readTablesInMainDb(doppelDbName, nameTableFromCycle);
	}

	qDebug() << "\n";
	counterForPercent = 0;

	return true;
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

	// высчитываем процент выполнения создания новых таблиц в новой БД

	counterForPercent++;

	double percentDouble = static_cast<double>(counterForPercent) / stringTablesArray.length() * 100.0;
	int percent = static_cast<int>(std::min(percentDouble, 100.0)); // вприсваиваем меньшее значение

	std::string tempForStdOut = QString::number(percent).toStdString() + "%   Add table " + tableNameTemp.toStdString();

	std::cout << "\r\x1b[2K" << tempForStdOut << std::flush; // делаем возврат корретки в текущей строке и затираем всю строку.

	structArrayForTable.clear();
}



void createTablesInDoppelDb(QString baseName, QString tableNameTemp)
{
	QSqlQuery createTableAndColumnInNewDb(masterDb);
	QString queryString;

	if (createFromCopy)
	{
		// Создаём новую таблицу в новой БД через копирование структуры с данными или без них

		CopyWithData == true ? queryString = QString("SELECT * INTO %1.dbo.%2 FROM %3.dbo.%2 WHERE 1 = 0") : queryString = QString("SELECT * INTO %1.dbo.%2 FROM %3.dbo.%2")
			.arg(doppelDbName)
			.arg(tableNameTemp)
			.arg(getDataBaseName(mainDb.databaseName()));

		if (!createTableAndColumnInNewDb.exec(queryString) || !createTableAndColumnInNewDb.next())
		{
			if (createTableAndColumnInNewDb.lastError().isValid())
			{
				std::cout << "Error in createTablesInDoppelDb when try create new table(" << tableNameTemp.toStdString() << "): " << createTableAndColumnInNewDb.lastError().text().toStdString() << std::endl;
				structArrayForTable.clear();
				return;
			}
		}
	}
	else
	{
		// Проверяем наличие дубликатов таблиц в новой БД

		if (checkDuplicateTableBool)
		{
			queryString = QString("SELECT * FROM %1.INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = '%2'")
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
		}

		// Создаём новую таблицу в новой БД через системные таблицы

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
}



void dropDataBase(QString baseName)
{
	doppelDb.close();

	if (baseName == "")
	{
		qDebug() << "baseName is void. Try again and check your params";
		return;
	}

	QSqlQuery dropDataBaseQuery(masterDb);
	QString queryString = QString("DROP DATABASE %1").arg(baseName);

	if (!dropDataBaseQuery.exec(queryString))
		std::cout << "Error in dropDataBase when try delete DB: " << dropDataBaseQuery.lastError().text().toStdString() << std::endl;
	else
		qDebug() << "DataBase " << baseName << " was deleted\n";
}



void createView(QString baseName)
{
	QSqlQuery createViewQuery(doppelDb);

	QList<QPair<QString, QString>>pairArrayForView;

	QString queryViewString = QString("SELECT TABLE_NAME, VIEW_DEFINITION FROM %1.INFORMATION_SCHEMA.VIEWS WHERE TABLE_CATALOG = '%1' AND TABLE_SCHEMA = 'dbo'")
		.arg(baseName);

	if (!createViewQuery.exec(queryViewString) || !createViewQuery.next())
	{
		std::cout << "Error in createView when try to get all views: " << createViewQuery.lastError().text().toStdString() << std::endl;
		return;
	}
	else
	{
		do
		{
			pairArrayForView.push_back(qMakePair(createViewQuery.value(0).toString(), createViewQuery.value(1).toString()));
		} while (createViewQuery.next());
	}

	for (int valueCounter = 0; valueCounter < pairArrayForView.length(); valueCounter++)
	{
		queryViewString = QString("%1")
			.arg(pairArrayForView[valueCounter].second);

		if (!createViewQuery.exec(queryViewString))
		{
			std::cout << "Error in createView when try to create view: " << pairArrayForView[valueCounter].first.toStdString() << "\n" << createViewQuery.lastError().text().toStdString() << std::endl;
			return;
		}
		else
		{
			counterForPercent++;

			double percentDouble = static_cast<double>(counterForPercent) / pairArrayForView.length() * 100.0;
			int percent = static_cast<int>(std::min(percentDouble, 100.0)); // вприсваиваем меньшее значение

			std::string tempForStdOut = QString::number(percent).toStdString() + "%   Add view " + pairArrayForView[valueCounter].first.toStdString();

			std::cout << "\r\x1b[2K" << tempForStdOut << std::flush;// делаем возврат корретки в текущей строке и затираем всю строку.


		}
	}

	qDebug() << "";

	counterForPercent = 0;
}



void createLogin()
{
	QSqlQuery genQuery(masterDb);

	QStringList createScripts;
	// SELECT 'CREATE LOGIN [' + name + '_doppelganger' + '] WITH PASSWORD = ' + 
	QString tempQuery = QString(R"(
    SELECT 'CREATE LOGIN [' + name + '] WITH PASSWORD = ' + 
       CONVERT(varchar(256), password_hash, 1) + ' HASHED, ' + ' CHECK_POLICY = OFF, ' +
       CASE WHEN default_database_name IS NOT NULL 
            THEN ' DEFAULT_DATABASE=[' + default_database_name + ']'
            ELSE '' END
FROM sys.sql_logins WHERE name NOT like '%_doppelganger' and type = 'S' AND default_database_name != 'master')");

	if (!genQuery.exec(tempQuery) || !genQuery.next())
	{
		std::cout << "Error in createRole when try to get all script: " << genQuery.lastError().text().toStdString();
	}
	else
	{
		qDebug() << "";

		do
		{
			QString script = genQuery.value(0).toString();

			if (!script.isEmpty())
			{
				createScripts << script;
			}
			else
			{
				qDebug() << "Query is empty";
			}

		} while (genQuery.next());
	}

	QSqlQuery applyQuery(doppelDb);

	for (const QString& script : createScripts)
	{
		if (!applyQuery.exec(script))
		{
			qDebug() << "Error in createRole when try create new login:" << applyQuery.lastError().text();
			qDebug() << "Failed script:" << script << "\n";
		}
		else
			qDebug() << applyQuery.lastQuery() << "\n";

	}
}



void dropRole()
{
	QSqlQuery getQuery(masterDb);
	QSqlQuery dropQuery(masterDb);

	QString tempQueryString = QString("SELECT NAME FROM master.sys.sql_logins WHERE NAME LIKE '%_doppelganger'");

	if (!getQuery.exec(tempQueryString) || !getQuery.next())
	{
		qDebug() << getQuery.lastQuery();
		std::cout << "Error in dropRole when try to get all login with _dopprlganger " + getQuery.lastError().text().toStdString() << std::endl;
	}
	else
	{
		do {
			if (!dropQuery.exec(QString("DROP LOGIN %1").arg(getQuery.value(0).toString())))
			{
				std::cout << "Error in dropRole when try delet login " << getQuery.value(0).toString().toStdString() << ": " << dropQuery.lastError().text().toStdString() << std::endl;
			}
			else
				qDebug() << "Login " << getQuery.value(0).toString() << " was drop from master base\n";
		} while (getQuery.next());
	}
}



void createUser()
{
	QSqlQuery getQuery(doppelDb);
	QSqlQuery createQuery(doppelDb);


	QString tempQueryString = QString("SELECT NAME FROM sys.sql_logins WHERE default_database_name != 'master'");

	if (!getQuery.exec(tempQueryString) || !getQuery.next())
	{
		qDebug() << getQuery.lastQuery();
		std::cout << "Error in createUser when try to get all login not master " + getQuery.lastError().text().toStdString() << std::endl;
	}
	else
	{
		do {
			if (!createQuery.exec(QString("CREATE USER %1 FOR LOGIN %1").arg(getQuery.value(0).toString())))
			{
				std::cout << "Error in createUser when try create user " << getQuery.value(0).toString().toStdString() << ": " << createQuery.lastError().text().toStdString() << std::endl;
			}
			else
				qDebug() << "User " << getQuery.value(0).toString() << " was create for " << doppelDbName << "\n";
		} while (getQuery.next());
	}
}