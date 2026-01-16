#include <QtCore/QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <qsqlquery>
#include <iostream>
#include <QRegularExpression>
#include <qfile.h>



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

struct ParamsForSql
{
	QString typeOfMainDb = "";
	QString typeOfAuthorizationMainDb = "";
	QString hostOfMainDb = "";
	QString NameOfMainDb = "";
	QString loginMainDb = "";
	QString passMainDb = "";

	QString typeOfMasterDb = "";
	QString typeOfAuthorizationMasterDb = "";
	QString hostOfMasterDb = "";
	QString NameOfMasterDb = "";
	QString loginMasterDb = "";
	QString passMasterDb = "";

	QString typeOfDoppelDb = "";
	QString typeOfAuthorizationDoppelDb = "";
	QString hostOfDoppelDb = "";
	QString NameOfDoppelDb = "";
	QString loginDoppelDb = "";
	QString passDoppelDb = "";
};

bool connectDataBase(QSqlDatabase& tempDbConnection, bool masterBool, bool doppelBool);
bool getTablesArray();
QString getDataBaseName(QString paramString);
bool createDoppelDbFromMain(QString tempName);
void readTablesInMainDb(QString baseName, QString tableNameTemp);
void createTablesInDoppelDb(QString baseName, QString tableNameTemp);
void dropDataBase(QString baseName);
void createView(QString baseName);
void createLogin();
bool createUser();
void dropRole();
QString validateHost();
QString validateBaseLoginPass(int number);
void addParamForDbConnection(QSqlDatabase& tempDbConnection, QString nameConnection);
QString validateTypeOfColumn(QString any, QString maxLength);
void addValueInNewDb(QList<TableColumnStruct> any, QString table, QString progress);
void readDefaultConfig();
void writeCurrent();

QList<QString>stringTablesArray;

QSqlDatabase mainDb;
QSqlDatabase masterDb;
QSqlDatabase doppelDb;

QString mainDbName;
QString masterDbName;
QString doppelDbName;
QString temporaryDbName;

int counterForPercent = 1;
bool checkDuplicateTableBool = false;
bool createFromCopy = false;
bool CopyWithData = false;
bool paramForConnectionFromFile = false;

int testCounter;
int sliderIndexForDefaultParams = 0;

QList<TableColumnStruct>structArrayForTable;

ParamsForSql paramsDefault;

QStringList paramStringList;



int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "russian");

	qDebug() << "All drivers for connection: " << QSqlDatabase::drivers() << "\n";

	QCoreApplication app(argc, argv);

	//checkDuplicateTableBool = true; /////////////////////////
	//createFromCopy = true; /////////////////////////////

	qDebug() << "Trying connecting to main DataBase (Donor)\n";

	if (connectDataBase(mainDb, 0, 0))
	{
		if (getTablesArray())
		{
			qDebug() << "Trying connecting to master DataBase (Local master)\n";

			if (connectDataBase(masterDb, 1, 0))
			{
				if (createDoppelDbFromMain(mainDbName))
				{
					qDebug() << "Trying create logins for " << doppelDbName << "\n";

					createLogin();

					qDebug() << "Trying create users for " << doppelDbName << "\n";

					if (createUser())
					{
						dropDataBase(doppelDbName); ///////////////////
						qDebug() << "Trying connecting to doppelganger DataBase\n";

						if (connectDataBase(doppelDb, 0, 1))
						{
							createView(mainDbName);
						}
						else
							qDebug() << "Check your setting for connect to doppelDb DataBase and try again";
					}
					else
						qDebug() << "Fail when trying to create users for " << doppelDbName << "\n";
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
	// Производим подключение к целевой БД. Запрашиваем и валидируем данные для пордключения к БД

	if (!masterBool)
	{
		if (!doppelBool)
		{
			addParamForDbConnection(tempDbConnection, "mainDbConn");
		}
		else
		{
			addParamForDbConnection(tempDbConnection, "doppelConn");
		}
	}
	else
	{
		addParamForDbConnection(tempDbConnection, "masterConn");
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
		if (masterBool)
		{
			masterDbName = temporaryDbName;
		}
		if (!masterBool && !doppelBool)
		{
			mainDbName = temporaryDbName;
		}
		if (doppelBool)
		{
			doppelDbName = temporaryDbName;
		}

		qDebug() << "\nDataBase is CONNECT to " << temporaryDbName << " with connection name = " << tempDbConnection.connectionName() << '\n';

		temporaryDbName = "";

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
	int percent = static_cast<int>(std::min(percentDouble, 100.0)); // присваиваем меньшее значение чтобы не превышать 100

	std::string tempForStdOut = QString::number(percent).toStdString() + "%   Add table " + tableNameTemp.toStdString();

	std::cout << "\r\x1b[2K" << tempForStdOut << std::flush; // делаем возврат корретки в текущей строке и затираем всю строку.

	//testCounter++;///////////////////////////////////////////////////////////////////

	//if (testCounter <= 7) addValueInNewDb(structArrayForTable, tableNameTemp, QString::fromStdString(tempForStdOut));////


	//if (tableNameTemp == "AL_CATEGORY") addValueInNewDb(structArrayForTable, tableNameTemp, QString::fromStdString(tempForStdOut));/////////////////////////////////////////////////
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
				std::cout << "\nError in createTablesInDoppelDb when try create new table(" << tableNameTemp.toStdString() << "): " << createTableAndColumnInNewDb.lastError().text().toStdString() << "\n" << createTableAndColumnInNewDb.lastQuery().toStdString() << std::endl;
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
			.arg('[' + baseName + ']')
			.arg('[' + tableNameTemp + ']')
			.arg('[' + structArrayForTable[0].ColumnName + ']')
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
				.arg('[' + baseName + ']')
				.arg('[' + tableNameTemp + ']')
				.arg('[' + structArrayForTable[counterColumn].ColumnName + ']')
				.arg(validateTypeOfColumn(structArrayForTable[counterColumn].dataType, QString::number(structArrayForTable[counterColumn].characterMaximumLength)))
				.arg(structArrayForTable[counterColumn].isNullable == "YES" ? "" : "NOT NULL");

			if (!createTableAndColumnInNewDb.exec(queryString))
			{
				if (createTableAndColumnInNewDb.lastError().isValid())
				{
					std::cout << "\nError in createTablesInDoppelDb when try add new column: " << createTableAndColumnInNewDb.lastError().text().toStdString() << "\n" << createTableAndColumnInNewDb.lastQuery().toStdString() << std::endl;
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

	std::string acceptDelete;
	std::cout << "Do you want to delete " + baseName.toStdString() + " ? (Y/y - delete DB / N/n - not delete DB)" << std::endl;

	do {
		std::cin >> acceptDelete;
		if (acceptDelete == "Y" || acceptDelete == "y" || acceptDelete == "N" || acceptDelete == "n") break;
		qDebug() << "Incorrect symbol. Try again";
	} while (true);

	if (acceptDelete == "Y" || acceptDelete == "y")
	{
		if (baseName == "")
		{
			qDebug() << "baseName is void. Try again and check your params";
			return;
		}

		QSqlQuery dropDataBaseQuery(masterDb);
		QString queryString = QString("DROP DATABASE %1").arg(baseName);

		dropDataBaseQuery.exec("USE master;");

		if (!dropDataBaseQuery.exec(QString("EXEC sp_removedbreplication '%1';").arg(baseName)))
			std::cout << "Error in dropDataBase when try close replication: " << dropDataBaseQuery.lastError().text().toStdString() << std::endl;


		if (!dropDataBaseQuery.exec(queryString))
			std::cout << "Error in dropDataBase when try delete DB: " << dropDataBaseQuery.lastError().text().toStdString() << std::endl;
		else
			qDebug() << "DataBase " << baseName << " was deleted\n";
	}
}



void createView(QString baseName)
{
	QSqlQuery readViewQuery(mainDb);
	QSqlQuery createViewQuery(doppelDb);

	QList<QPair<QString, QString>>pairArrayForView;

	QString queryViewString = QString("SELECT TABLE_NAME, VIEW_DEFINITION FROM %1.INFORMATION_SCHEMA.VIEWS WHERE TABLE_CATALOG = '%1' AND TABLE_SCHEMA = 'dbo'")
		.arg(baseName);

	if (!readViewQuery.exec(queryViewString) || !readViewQuery.next())
	{
		std::cout << "Error in createView when try to get all views: " << readViewQuery.lastError().text().toStdString() << std::endl;
		return;
	}
	else
	{
		do
		{
			pairArrayForView.push_back(qMakePair(readViewQuery.value(0).toString(), readViewQuery.value(1).toString()));
		} while (readViewQuery.next());
	}

	for (int valueCounter = 0; valueCounter < pairArrayForView.length(); valueCounter++)
	{
		queryViewString = QString("%1")
			.arg(pairArrayForView[valueCounter].second);

		if (!createViewQuery.exec(queryViewString))
		{
			std::cout << "Error in createView when try to create view: " << pairArrayForView[valueCounter].first.toStdString() << "\n" << createViewQuery.lastError().text().toStdString() << "\n" << createViewQuery.lastQuery().toStdString() << std::endl;
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

	qDebug() << "\n";

	counterForPercent = 0;
}



void createLogin()
{
	// Формируем SQL запрос за добавление в сервер рецепиент логинов от донорской БД (Учетной записи требуется уровень роли сервера не меньше сисадмин. хотя бы на вермя переноса)

	QSqlQuery genQuery(mainDb);

	QStringList createScripts;

	QString tempQuery = QString(R"(
    SELECT 'CREATE LOGIN [' + name + '] WITH PASSWORD = ' + 
       CONVERT(varchar(256), password_hash, 1) + ' HASHED, ' + ' CHECK_POLICY = OFF, ' +
       CASE WHEN default_database_name IS NOT NULL 
            THEN ' DEFAULT_DATABASE=[' + default_database_name + '_doppelganger' + ']'
            ELSE '' END
FROM sys.sql_logins WHERE name NOT like '%_doppelganger' and type = 'S' AND default_database_name != 'master' AND default_database_name = '%1')").arg(mainDbName);

	if (!genQuery.exec(tempQuery) || !genQuery.next())
	{
		if (genQuery.lastError().isValid())
			std::cout << "Error in createRole when try to get all script: " << genQuery.lastError().text().toStdString() << genQuery.lastQuery().toStdString() << "\n\n" << std::endl;
		else
			qDebug() << "Unknown error. Try to check your params";
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

	QSqlQuery applyQuery(masterDb);

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



bool createUser()
{
	// создаём пользователей и связываем их с логинами сервера которые были созданы ранее

	QSqlQuery getQuery(mainDb);
	QSqlQuery createQuery(masterDb);

	QString tempQueryString = QString("SELECT NAME FROM sys.sql_logins WHERE default_database_name != 'master' AND default_database_name = '%1'").arg(mainDbName);

	if (!getQuery.exec(tempQueryString) || !getQuery.next())
	{
		std::cout << "Error in createUser when try to get all login not master " + getQuery.lastError().text().toStdString() << std::endl;
		qDebug() << getQuery.lastQuery();
	}
	else
	{
		// даём дополнительные права на выполнение DDL запросов для создания представлений и прочего

		do {
			if (!createQuery.exec(QString("USE %1 CREATE USER %2 FOR LOGIN %2 ALTER ROLE db_datareader ADD MEMBER %2 ALTER ROLE db_datawriter ADD MEMBER %2 ALTER ROLE db_ddladmin ADD MEMBER %2;")
				.arg(doppelDbName)
				.arg(getQuery.value(0).toString())))
			{
				std::cout << "Error in createUser when try create user " << getQuery.value(0).toString().toStdString() << ": " << createQuery.lastError().text().toStdString() << "\n" << createQuery.lastQuery().toStdString() << std::endl;
			}
			else
				qDebug() << "User " << getQuery.value(0).toString() << " was create for " << doppelDbName << "\n";
		} while (getQuery.next());

		return true;
	}
}



QString validateHost()
{
	// Запрашиваем и валидируем хост

	std::string host;
	std::cout << "What host use to connect (IP,Port)" << std::endl;

	do {
		if (!paramForConnectionFromFile)
			std::cin >> host;
		else
		{
			host = paramStringList[3 + sliderIndexForDefaultParams].toStdString();
			qDebug() << QString::fromStdString(host);
		}

		QRegularExpression strPattern(QString(R"([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\,[0-9]{2,5})"));

		QRegularExpressionMatch matchReg = strPattern.match(QString::fromStdString(host));

		if (matchReg.hasMatch())
		{
			host = matchReg.captured().toStdString();
			break;
		}

		qDebug() << "Not find in your messege host with format \"IP,Port\". Try again";
	} while (true);

	return QString::fromStdString(host);
}






QString validateBaseLoginPass(int number)
{
	std::string host;

	switch (number)
	{
	case(1):
	{
		std::cout << "What name of DataBase?" << std::endl;
		break;
	}

	case(2):
	{
		std::cout << "What login?" << std::endl;
		break;
	}

	case(3):
	{
		std::cout << "What password?" << std::endl;
		break;
	}
	}

	do {

		if (!paramForConnectionFromFile)
			std::cin >> host;
		else
		{
			host = paramStringList[3 + number + sliderIndexForDefaultParams].toStdString();
			qDebug() << QString::fromStdString(host);
		}

		if (host.length() < 50)
		{
			if (number == 1)
				temporaryDbName = QString::fromStdString(host);
			break;
		}

		qDebug() << "Too much length of you messege. Try again;";
	} while (true);

	return QString::fromStdString(host);
}



void addParamForDbConnection(QSqlDatabase& tempDbConnection, QString nameConnection)
{
	// Запрашиваем и валидируем данные для пордключения к БД

	if (!paramForConnectionFromFile)
	{
		std::string readFromFile;
		std::cout << "Do you want to read params for connection from file? (Y/y / N/n)" << std::endl;

		do {
			std::cin >> readFromFile;
			if (readFromFile == "Y" || readFromFile == "y" || readFromFile == "N" || readFromFile == "n") break;
			qDebug() << "Incorrect symbol. Try again";
		} while (true);

		if (readFromFile == "Y" || readFromFile == "y")
		{
			paramForConnectionFromFile = true;
			if (nameConnection == "mainDbConn");
			sliderIndexForDefaultParams = 0;
			if (nameConnection == "masterConn");
			sliderIndexForDefaultParams = 6;
			if (nameConnection == "doppelConn");
			sliderIndexForDefaultParams = 13;
			readDefaultConfig();
		}
		else
			paramForConnectionFromFile = false;
	}

	std::string typeOfDb;
	std::cout << "What type of DataBase will connect now? (S/s - SQL Server / P/p - PostgreSQL)" << std::endl;

	do {
		if (!paramForConnectionFromFile)
			std::cin >> typeOfDb;
		else
		{
			typeOfDb = paramStringList[0 + sliderIndexForDefaultParams].toStdString();
			qDebug() << QString::fromStdString(typeOfDb);
		}

		if (typeOfDb == "S" || typeOfDb == "s" || typeOfDb == "P" || typeOfDb == "p") break;
		qDebug() << "Incorrect symbol. Try again";

	} while (true);

	if (typeOfDb == "S" || typeOfDb == "s")
	{
		std::string typeOfAuthentication;
		std::cout << "What type of Authentication will use? (S/s - SQL authentication / W/w - Windows authentication)" << std::endl;

		do {
			if (!paramForConnectionFromFile)
				std::cin >> typeOfAuthentication;
			else
			{
				typeOfAuthentication = paramStringList[1 + sliderIndexForDefaultParams].toStdString();
				qDebug() << QString::fromStdString(typeOfAuthentication);
			}

			if (typeOfAuthentication == "S" || typeOfAuthentication == "s" || typeOfAuthentication == "W" || typeOfAuthentication == "w") break;
			qDebug() << "Incorrect symbol. Try again";

		} while (true);

		if (typeOfAuthentication == "S" || typeOfAuthentication == "s")
		{

			tempDbConnection = QSqlDatabase::addDatabase("QODBC", nameConnection); // Работаем используя QODBC (SQL Server) и отдельную строку. Методы частично работают лишь с пользовательским доменом.. 
			tempDbConnection.setDatabaseName(
				QString(R"(
					DRIVER={ODBC Driver 17 for SQL Server};
					Server=%1;
					Database=%2;
					Uid=%3;
					Pwd=%4;
)")
.arg(validateHost())
.arg(validateBaseLoginPass(1))
.arg(validateBaseLoginPass(2))
.arg(validateBaseLoginPass(3))

); // "DRIVER={SQL Server};" - алтернативный вариант написания c помощью старого драйвера (не рекомендуется)
		}
		else
		{
			tempDbConnection = QSqlDatabase::addDatabase("QODBC", nameConnection);
			QString connStr = QString("DRIVER={ODBC Driver 17 for SQL Server};Server=%1;DATABASE=%2;Trusted_Connection=yes;")
				.arg(validateHost())
				.arg(validateBaseLoginPass(1));
			tempDbConnection.setDatabaseName(connStr);
		}
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



QString validateTypeOfColumn(QString any, QString maxLength)
{
	QString returnString = "";

	if (any == "varchar" || any == "nvarchar")
	{
		if (any == "varchar")
		{
			returnString += "varchar(";

			if (maxLength == "-1")
				returnString += "max)";
			else
				returnString += maxLength + ")";
		}

		if (any == "nvarchar")
		{
			returnString += "nvarchar(";

			if (maxLength == "-1")
				returnString += "max)";
			else
				returnString += maxLength + ")";
		}
	}
	else
		returnString = any;

	return returnString;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
void addValueInNewDb(QList<TableColumnStruct> any, QString table, QString progress)
{
	QSqlQuery selectQuery(mainDb);
	QSqlQuery insertQuery(masterDb);

	QString identityInsertString = QString("SET IDENTITY_INSERT [%1].[dbo].[%2] OFF")
		.arg(mainDbName)
		.arg(table);

	if (!insertQuery.exec(identityInsertString))
	{
		std::cout << "\nError in addValueInNewDb when try identity insert off " + table.toStdString() + ". " << selectQuery.lastError().text().toStdString() << std::endl;
		qDebug() << selectQuery.lastQuery();
		return;
	}


	QString queryInsertString = QString("INSERT INTO [%1].[dbo].[%2](")
		.arg(doppelDbName)
		.arg(table);

	for (auto& val : any)
	{
		queryInsertString += val.ColumnName + ',';
	}

	queryInsertString.chop(1);

	queryInsertString += ") VALUES(";

	QString querySelectString = QString("SELECT * FROM [%1].[dbo].[%2]")
		.arg(mainDbName)
		.arg(table);

	if (!selectQuery.exec(querySelectString))
	{
		std::cout << "\nError in addValueInNewDb when try to get values from table " + table.toStdString() + ". " << selectQuery.lastError().text().toStdString() << std::endl;
		qDebug() << selectQuery.lastQuery();
		return;
	}

	selectQuery.next();

	if (!selectQuery.isValid())
	{
		qDebug() << " - Read values is done but table is empty";
		return;
	}
	else
	{
		selectQuery.last();
		long long countOfRowInQuery = selectQuery.at();
		selectQuery.first();

		do {
			QString temporaryInsertForSingleString = queryInsertString;
			/*
			// Подготовленный запрос
			QSqlQuery query;
			query.prepare("INSERT INTO AL_PROTOCOL (MESSAGE, SOURCE, SEVERITY) VALUES (?, ?, ?)");

			// Привязка значений — Qt сам экранирует
			QString message = "текст с 'кавычками' и \"двойными\"";
			QString source = "ASTUE (172.22.22.4)";
			int severity = 500;

			query.addBindValue(message);  // Автоэкранирование
			query.addBindValue(source);
			query.addBindValue(severity);
			query.exec();
			*/

			for (int counter = 0; counter < structArrayForTable.length(); counter++)
			{
				/*
				if (structArrayForTable[counter].dataType == "varchar" || structArrayForTable[counter].dataType == "nvarchar" || structArrayForTable[counter].dataType == "datetime")
					temporaryInsertForSingleString += "'" + selectQuery.value(counter).toString() + "'" + ',';
				else
					temporaryInsertForSingleString += selectQuery.value(counter).toString() + ',';
					*/


				temporaryInsertForSingleString += "?, ";
			}

			insertQuery.prepare(temporaryInsertForSingleString);

			for (int counter = 0; counter < structArrayForTable.length(); counter++)
			{
				insertQuery.addBindValue(selectQuery.value(counter).toString());
			}


			temporaryInsertForSingleString.chop(2);
			temporaryInsertForSingleString += ')';

			if (!insertQuery.exec(temporaryInsertForSingleString))
			{
				std::cout << "\n\nError in addValueInNewDb when try to insert values in table " + doppelDbName.toStdString() + ". " << insertQuery.lastError().text().toStdString() << std::endl;
				qDebug() << insertQuery.lastQuery() << "\n";
			}
			else
			{
				QString progressString = progress + " - Values was added into " + table + " [ " + QString::number(selectQuery.at()) + " / " + QString::number(countOfRowInQuery) + " ] ";
				std::cout << "\r\x1b[2K" << progressString.toStdString() << std::flush; // делаем возврат корретки в текущей строке и затираем всю строку.
			}
		} while (selectQuery.next());

	}

	identityInsertString = QString("SET IDENTITY_INSERT [%1].[dbo].[%2] ON")
		.arg(mainDbName)
		.arg(table);

	if (!insertQuery.exec(identityInsertString))
	{
		std::cout << "\nError in addValueInNewDb when try identity insert on " + table.toStdString() + ". " << selectQuery.lastError().text().toStdString() << std::endl;
		qDebug() << selectQuery.lastQuery();
		return;
	}

}



void readDefaultConfig()
{
	QFile file(QCoreApplication::applicationDirPath() + "\\config.txt");

	if (!file.open(QIODevice::ReadOnly))
	{
		qDebug() << "Dont find config file. Add file with parameters.";
		return;
	}

	QTextStream in(&file);

	int countParam = 0;

	// Считываем файл строка за строкой

	while (!in.atEnd()) // метод atEnd() возвращает true, если в потоке больше нет данных для чтения
	{
		QString line = in.readLine(); // метод readLine() считывает одну строку из потока
		++countParam;
		QString temporary;

		for (auto& val : line)
		{
			temporary += val;
		}
		qDebug() << temporary;
		//main
		switch (countParam)
		{

		case(1):
		{
			paramsDefault.typeOfMainDb = temporary;
			break;
		}
		case(2):
		{
			paramsDefault.typeOfAuthorizationMainDb = temporary;
			break;
		}
		case(3):
		{
			paramsDefault.hostOfMainDb = temporary;
			break;
		}
		case(4):
		{
			paramsDefault.NameOfMainDb = temporary;
			break;
		}
		case(5):
		{
			paramsDefault.loginMainDb = temporary;
			break;
		}
		case(6):
		{
			paramsDefault.passMainDb = temporary;
			break;
		}

		//master
		case(7):
		{
			paramsDefault.typeOfMasterDb = temporary;
			break;
		}
		case(8):
		{
			paramsDefault.typeOfAuthorizationMasterDb = temporary;
			break;
		}
		case(9):
		{
			paramsDefault.hostOfMasterDb = temporary;
			break;
		}
		case(10):
		{
			paramsDefault.NameOfMasterDb = temporary;
			break;
		}
		case(11):
		{
			paramsDefault.loginMasterDb = temporary;
			break;
		}
		case(12):
		{
			paramsDefault.passMasterDb = temporary;
			break;
		}

		// doppel
		case(13):
		{
			paramsDefault.typeOfDoppelDb = temporary;
			break;
		}
		case(14):
		{
			paramsDefault.typeOfAuthorizationDoppelDb = temporary;
			break;
		}
		case(15):
		{
			paramsDefault.hostOfDoppelDb = temporary;
			break;
		}
		case(16):
		{
			paramsDefault.NameOfDoppelDb = temporary;
			break;
		}
		case(17):
		{
			paramsDefault.loginDoppelDb = temporary;
			break;
		}
		case(18):
		{
			paramsDefault.passDoppelDb = temporary;
			break;
		}
		}

		paramStringList << temporary;
	}

	file.close();
}

void writeCurrent()
{
	QFile file(QCoreApplication::applicationDirPath() + "\\config.txt");

	// Открываем файл в режиме "Только для записи"
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QTextStream out(&file); // поток записываемых данных направляем в файл

		// Для записи данных в файл используем оператор <<
		out << paramsDefault.typeOfMainDb << Qt::endl;
		out << paramsDefault.typeOfAuthorizationMainDb << Qt::endl;
		out << paramsDefault.hostOfMainDb << Qt::endl;
		out << paramsDefault.NameOfMainDb << Qt::endl;
		out << paramsDefault.loginMainDb << Qt::endl;
		out << paramsDefault.passMainDb << Qt::endl;

		out << paramsDefault.typeOfMasterDb << Qt::endl;
		out << paramsDefault.typeOfAuthorizationMasterDb << Qt::endl;
		out << paramsDefault.hostOfMasterDb << Qt::endl;
		out << paramsDefault.NameOfMasterDb << Qt::endl;
		out << paramsDefault.loginMasterDb << Qt::endl;
		out << paramsDefault.passMasterDb << Qt::endl;

		out << paramsDefault.typeOfDoppelDb << Qt::endl;
		out << paramsDefault.typeOfAuthorizationDoppelDb << Qt::endl;
		out << paramsDefault.hostOfDoppelDb << Qt::endl;
		out << paramsDefault.NameOfDoppelDb << Qt::endl;
		out << paramsDefault.loginDoppelDb << Qt::endl;
		out << paramsDefault.passDoppelDb << Qt::endl;
	}
	else
	{
		qWarning("Could not open file");
	}

	file.close();
}