#include <QtCore/QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <qsqlquery>
#include <iostream>
#include <QRegularExpression>
#include <qfile.h>
#include <QTextCodec>



struct TableColumnStruct
{
	QString tableCatalog = "";
	QString tableSchema = "";
	QString tableName = "";
	QString ColumnName = "";
	int ordinalPosition = 0;;
	QString columnDefault = "";
	QString isNullable = "";
	QString dataType = "";
	int characterMaximumLength = 0;
	int characterOctetLength = 0;
	int numericPrecision = 0;
	int numericPrecisionRadix = 0;
	int numericScale = 0;
	int dateTimePrecision = 0;
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
void createFullBdFromCopy(QString tableNameTemp);
void checkDuplicateTableInNewBd(QString baseName, QString tableNameTemp);
void createFK();
void createIndexInNewTable(QString tempTable);

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
QStringList checkSpecialTypeArray;


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
						qDebug() << "Trying connecting to doppelganger DataBase\n";

						if (connectDataBase(doppelDb, 0, 1))
						{
							createView(mainDbName);
							createFK();
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
		/*
		/////////////////////////////////////////////////////////////////////////////////////////////////

		QString querySelectString = QString("SELECT TOP(1) ");
		QString ColumnName = "Definition";

				querySelectString += "CAST([" + ColumnName + "] AS NVARCHAR(MAX)) AS " + ColumnName + ',';


		querySelectString.chop(1);

		querySelectString += QString(" FROM [%1].[dbo].[%2]")
			.arg("ProSoft_ASKUE-Utek")
			.arg("LossesParameters");



		QSqlQuery testQuery(tempDbConnection);

		//testQuery.exec(QString("SELECT TOP(1) CAST([Definition] AS NVARCHAR(MAX)) AS Definition FROM [ProSoft_ASKUE-Utek].[dbo].[LossesParameters]"));
		testQuery.exec(querySelectString);
		testQuery.next();

		qDebug() << testQuery.value(0).toString();

		return false;
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		*/
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
			std::cout << "Error in createDoppelDbFromMain when check new DB: " << createBase.lastError().text().toStdString() << std::endl;
			qDebug() << createBase.lastQuery();
			return false;
		}

		QString createDataBaseStringQuery = QString("CREATE DATABASE [%1]").arg(doppelDbName); // создание БД нельзя в SQL Server провести с использованием placeholders. Подготовленные запросы не пройдут.

		if (!createBase.exec(createDataBaseStringQuery))
		{
			qDebug() << "Error in createDoppelDbFromMain when create new DB: " << doppelDbName + " wasnt create because:" << "\n";
			std::cout << createBase.lastError().text().toStdString() << std::endl;
			std::cout << createBase.lastError().databaseText().toStdString() << std::endl;
			std::cout << createBase.lastError().driverText().toStdString() << std::endl;
			qDebug() << createBase.lastQuery();

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
			checkSpecialTypeArray << tempStruct.dataType;

		} while (createTableAndColumn.next());

		createTablesInDoppelDb(baseName, tableNameTemp);
	}

	// высчитываем процент выполнения создания новых таблиц в новой БД

	counterForPercent++;

	double percentDouble = static_cast<double>(counterForPercent) / stringTablesArray.length() * 100.0;
	int percent = static_cast<int>(std::min(percentDouble, 100.0)); // присваиваем меньшее значение чтобы не превышать 100

	std::string tempForStdOut = QString::number(percent).toStdString() + "%   Add table " + tableNameTemp.toStdString();

	std::cout << "\r\x1b[2K" << tempForStdOut << std::flush; // делаем возврат корретки в текущей строке и затираем всю строку.

	//addValueInNewDb(structArrayForTable, tableNameTemp, QString::fromStdString(tempForStdOut));

	structArrayForTable.clear();
	checkSpecialTypeArray.clear();

	createIndexInNewTable(tableNameTemp); //////////////////////////test но вероятно тут и останется
}



void createFullBdFromCopy(QString tableNameTemp)
{
	QSqlQuery createTableAndColumnInNewDb(masterDb);
	QString queryString;

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



void checkDuplicateTableInNewBd(QString baseName, QString tableNameTemp)
{
	// Проверяем наличие дубликатов таблиц в новой БД

	QSqlQuery createTableAndColumnInNewDb(masterDb);
	QString queryString;

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
}



void createTablesInDoppelDb(QString baseName, QString tableNameTemp)
{
	QSqlQuery createTableAndColumnInNewDb(masterDb);
	QString queryString;

	if (createFromCopy)
	{
		createFullBdFromCopy(tableNameTemp);
	}
	else
	{
		checkDuplicateTableInNewBd(baseName, tableNameTemp);

		QSqlQuery identityQueryFromMain(mainDb);
		QSqlQuery getPkName(mainDb);
		QString tempPrimaryKey = "";
		QString tempFoGetPk;
		bool identity = false;
		bool primary = false;
		QString specialColuimnForCompareIdentity;
		QString specialColuimnForComparePK;
		QString nameColumnForPK;
		QString manyComponentPK;

		// Получаем информацию на предмет наличия IDENTITY_COLUMN
		// sql_variant требуется преобразовать в запросе в целевой тип данных т.к. иначе буду проблемы с получением значений

		QString checkIncrementValue = QString("SELECT name, CAST(seed_value AS INT) AS seed_value, CAST(increment_value AS INT) AS increment_value FROM [%1].sys.identity_columns WHERE OBJECT_NAME(object_id) = '%2'")
			.arg(mainDbName)
			.arg(tableNameTemp);
		if (!identityQueryFromMain.exec(checkIncrementValue) || !identityQueryFromMain.next())
		{
			if (identityQueryFromMain.lastError().isValid())
			{
				std::cout << "\nError in addValueInNewDb when try to get all identity_columns in mainDB " + identityQueryFromMain.lastError().text().toStdString() << std::endl;
				qDebug() << identityQueryFromMain.lastQuery();
			}
		}
		else
		{
			if (identityQueryFromMain.isValid())
			{
				specialColuimnForCompareIdentity = identityQueryFromMain.value(0).toString();
				identity = true;
			}
			else
				identity = false;
		}

		// Получаем информацию на предмет наличия PRIMARY_KEY

		tempFoGetPk = QString("SELECT [name] FROM [%1].[sys].[key_constraints] WHERE OBJECT_NAME([parent_object_id]) = '%2'")
			.arg(mainDbName)
			.arg(tableNameTemp);

		if (!getPkName.exec(tempFoGetPk) || !getPkName.next())
		{
			if (getPkName.lastError().isValid())
			{
				std::cout << "\nError in addValueInNewDb when try to get name of PK " + getPkName.lastError().text().toStdString() << std::endl;
				qDebug() << getPkName.lastQuery();
			}
		}
		else
		{
			if (getPkName.isValid())
			{
				// Определяем является ли PK многосоставным и если является то формируем тело ключа

				specialColuimnForComparePK = getPkName.value(0).toString();

				tempFoGetPk = QString("SELECT [COLUMN_NAME], [CONSTRAINT_NAME] FROM [%1].[INFORMATION_SCHEMA].[KEY_COLUMN_USAGE] WHERE [TABLE_NAME] = '%2' AND CONSTRAINT_NAME = '%3'")
					.arg(mainDbName)
					.arg(tableNameTemp)
					.arg(specialColuimnForComparePK);

				if (!getPkName.exec(tempFoGetPk) || !getPkName.last())
				{
					if (getPkName.lastError().isValid())
					{
						std::cout << "\nError in addValueInNewDb when try to get count of PK and name of column" + getPkName.lastError().text().toStdString() << std::endl;
						qDebug() << getPkName.lastQuery();
					}
				}
				else
				{
					if (getPkName.at() == 0) // Первая запись занимает нулевую позицию
						nameColumnForPK = '[' + getPkName.value(0).toString() + ']';
					else
					{
						getPkName.first();
						nameColumnForPK = getPkName.value(0).toString();

						manyComponentPK += '[' + getPkName.value(0).toString() + ']';

						while (getPkName.next())
						{
							manyComponentPK += ", [" + getPkName.value(0).toString() + ']';
						}
					}
				}

				primary = true;
			}
			else
				primary = false;
		}

		// Определяем исход из полученных переменных характер первого столбца при создании таблицы

		if (primary && identity)
		{
			tempPrimaryKey = QString("IDENTITY(%1,%2) NOT NULL, CONSTRAINT [%3] PRIMARY KEY ([%4])")
				.arg(identityQueryFromMain.value(1).toString())
				.arg(identityQueryFromMain.value(2).toString())
				.arg(specialColuimnForComparePK)
				.arg(identityQueryFromMain.value(0).toString());
		}

		if (!primary && identity)
		{
			tempPrimaryKey = QString("IDENTITY(%1,%2) NOT NULL")
				.arg(identityQueryFromMain.value(1).toString())
				.arg(identityQueryFromMain.value(2).toString());
		}

		if (primary && !identity)
		{
			tempPrimaryKey = QString("NOT NULL, CONSTRAINT [%1] PRIMARY KEY ([%2])")
				.arg(specialColuimnForComparePK)
				.arg(structArrayForTable[0].ColumnName);
		}

		// Создаём новую таблицу в новой БД через системные таблицы

		QString tempForFirstColumn = '[' + structArrayForTable[0].ColumnName + ']';

		queryString = QString("CREATE TABLE %1.dbo.%2 (%3 %4 %5)")
			.arg('[' + baseName + ']')
			.arg('[' + tableNameTemp + ']')
			.arg('[' + structArrayForTable[0].ColumnName + ']')
			.arg(validateTypeOfColumn(structArrayForTable[0].dataType, QString::number(structArrayForTable[0].characterMaximumLength)))
			.arg(tempPrimaryKey == "" ? (structArrayForTable[0].isNullable == "YES" ? "" : "NOT NULL") : ((tempForFirstColumn == nameColumnForPK || structArrayForTable[0].ColumnName == specialColuimnForCompareIdentity) ? tempPrimaryKey : (structArrayForTable[0].isNullable == "YES" ? "" : "NOT NULL")));

		if (!createTableAndColumnInNewDb.exec(queryString) || !createTableAndColumnInNewDb.next())
		{
			if (createTableAndColumnInNewDb.lastError().isValid())
			{
				std::cout << "Error in createTablesInDoppelDb when try create new table(" << tableNameTemp.toStdString() << "): " << createTableAndColumnInNewDb.lastError().text().toStdString() << std::endl;
				qDebug() << createTableAndColumnInNewDb.lastQuery();
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
				.arg(tempPrimaryKey == "" ? (structArrayForTable[counterColumn].isNullable == "YES" ? "" : "NOT NULL") : ((structArrayForTable[counterColumn].ColumnName == specialColuimnForComparePK || structArrayForTable[counterColumn].ColumnName == specialColuimnForCompareIdentity) ? tempPrimaryKey : (structArrayForTable[counterColumn].isNullable == "YES" ? "" : "NOT NULL")));

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

		// Пересоздаём ключ если он был многокомпонентным предварительно его удалив, если он всё таки был создан

		if (manyComponentPK != "")
		{
			if (tempForFirstColumn == nameColumnForPK)
			{
				queryString = QString("ALTER TABLE %1.dbo.%2 DROP CONSTRAINT %3")
					.arg('[' + baseName + ']')
					.arg('[' + tableNameTemp + ']')
					.arg('[' + specialColuimnForComparePK + ']');

				if (!createTableAndColumnInNewDb.exec(queryString))
				{
					if (createTableAndColumnInNewDb.lastError().isValid())
					{
						std::cout << "\nError in createTablesInDoppelDb when try drop constrait: " << createTableAndColumnInNewDb.lastError().text().toStdString() << std::endl;
						qDebug() << createTableAndColumnInNewDb.lastQuery() << "\n";
					}

				}
			}

			queryString = QString("ALTER TABLE %1.dbo.%2 ADD CONSTRAINT %3 PRIMARY KEY (%4)")
				.arg('[' + baseName + ']')
				.arg('[' + tableNameTemp + ']')
				.arg('[' + specialColuimnForComparePK + ']')
				.arg(manyComponentPK);

			if (!createTableAndColumnInNewDb.exec(queryString))
			{
				if (createTableAndColumnInNewDb.lastError().isValid())
				{
					std::cout << "\nError in createTablesInDoppelDb when try drop constrait: " << createTableAndColumnInNewDb.lastError().text().toStdString() << std::endl;
					qDebug() << createTableAndColumnInNewDb.lastQuery() << "\n";
				}
			}
		}

		identityQueryFromMain.clear();
	}
}



void dropDataBase(QString baseName)
{
	// Опциональная функция удаления новосозданной БД на момент доработки приложения. В основном для того чтобы потом не удалять её вручную в дальнейшем. 

	doppelDb.close();
	mainDb.close();

	std::string acceptDelete;
	std::cout << "\nDo you want to delete " + baseName.toStdString() + " ? (Y/y - delete DB / N/n - not delete DB)" << std::endl;

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
		QString queryString = QString("DROP DATABASE [%1]").arg(baseName);

		dropDataBaseQuery.exec("USE master;");

		if (!dropDataBaseQuery.exec(QString("EXEC sp_removedbreplication '%1';").arg(baseName)))
		{
			std::cout << "Error in dropDataBase when try close replication: " << dropDataBaseQuery.lastError().text().toStdString() << std::endl;
			qDebug() << dropDataBaseQuery.lastQuery();
		}


		if (!dropDataBaseQuery.exec(queryString))
		{
			std::cout << "Error in dropDataBase when try delete DB: " << dropDataBaseQuery.lastError().text().toStdString() << std::endl;
			qDebug() << dropDataBaseQuery.lastQuery();
		}
		else
			qDebug() << "DataBase " << baseName << " was deleted\n";
	}

	masterDb.close();
}



void createView(QString baseName)
{
	QSqlQuery readViewQuery(mainDb);
	QSqlQuery createViewQuery(doppelDb);

	QList<QPair<QString, QString>>pairArrayForView;

	QString queryViewString = QString("SELECT TABLE_NAME, VIEW_DEFINITION FROM [%1].INFORMATION_SCHEMA.VIEWS WHERE TABLE_CATALOG = '%1' AND TABLE_SCHEMA = 'dbo'")
		.arg(baseName);

	if (!readViewQuery.exec(queryViewString) || !readViewQuery.next())
	{
		std::cout << "Error in createView when try to get all views: " << readViewQuery.lastError().text().toStdString() << std::endl;
		qDebug() << readViewQuery.lastQuery() << "\n";
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
			qDebug() << createViewQuery.lastQuery() << "\n";
		}
		else
		{
			counterForPercent++;

			double percentDouble = static_cast<double>(counterForPercent) / pairArrayForView.length() * 100.0;
			int percent = static_cast<int>(std::min(percentDouble, 100.0)); // вприсваиваем меньшее значение

			std::string tempForStdOut = QString::number(percent).toStdString() + "%   Add view " + pairArrayForView[valueCounter].first.toStdString();

			std::cout << "\r\x1b[2K" << tempForStdOut << std::flush; // делаем возврат корретки в текущей строке и затираем всю строку.
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

		return false;
	}
	else
	{
		// даём дополнительные права на выполнение DDL запросов для создания представлений и прочего

		do {
			if (!createQuery.exec(QString("USE [%1] CREATE USER %2 FOR LOGIN %2 ALTER ROLE db_datareader ADD MEMBER %2 ALTER ROLE db_datawriter ADD MEMBER %2 ALTER ROLE db_ddladmin ADD MEMBER %2;")
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
			host = paramStringList[2 + sliderIndexForDefaultParams].toStdString();
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
			host = paramStringList[2 + number + sliderIndexForDefaultParams].toStdString();
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
			readDefaultConfig();
		}
		else
			paramForConnectionFromFile = false;
	}

	if (nameConnection == "mainDbConn")
		sliderIndexForDefaultParams = 0;
	if (nameConnection == "masterConn")
		sliderIndexForDefaultParams = 6;
	if (nameConnection == "doppelConn")
		sliderIndexForDefaultParams = 12;

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
				qDebug() << getQuery.lastQuery();
			}
			else
				qDebug() << "Login " << getQuery.value(0).toString() << " was drop from master base\n";
		} while (getQuery.next());
	}
}



QString validateTypeOfColumn(QString any, QString maxLength)
{
	QString returnString = "";

	if (any == "varchar" || any == "nvarchar" || any == "char" || any == "varbinary" || any == "XML")
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

		if (any == "char")
		{
			returnString += "char(";

			if (maxLength == "-1")
				returnString += "max)";
			else
				returnString += maxLength + ")";
		}

		if (any == "varbinary")
		{
			returnString += "varbinary(";

			if (maxLength == "-1")
				returnString += "max)";
			else
				returnString += maxLength + ")";
		}

		if (any == "XML")
		{
			returnString += "XML(.)";
		}
	}
	else
		returnString = any;

	return returnString;
}



void addValueInNewDb(QList<TableColumnStruct> any, QString table, QString progress)
{
	QSqlQuery selectQuery(mainDb);
	QSqlQuery insertQuery(masterDb);
	QSqlQuery identitySelectQuery(mainDb);

	bool identityInster = false;
	QString identityInsertString;

	// Выявляем наличие данных в таблице
	QString querySelectString;
	bool xmlType = false;

	for (auto& val : any)
	{
		if (val.dataType == "xml")
		{
			xmlType = true;
		}
	}

	if (xmlType)
	{
		querySelectString = QString("SELECT TOP(1) ");

		for (auto& val : any)
		{
			if (val.dataType == "xml")
				querySelectString += "CAST([" + val.ColumnName + "] AS NVARCHAR(MAX)) AS " + val.ColumnName + ',';
			else
				querySelectString += '[' + val.ColumnName + "],";
		}

		querySelectString.chop(1);

		querySelectString += QString(" FROM [%1].[dbo].[%2]")
			.arg(mainDbName)
			.arg(table);
	}
	else
	{
		querySelectString = QString("SELECT TOP(1) * FROM [%1].[dbo].[%2]") // TEST (убрать ТОР(1) для внесения всех значений из таблицы или только части данных
			.arg(mainDbName)
			.arg(table);
	}

	if (!selectQuery.exec(querySelectString))
	{
		std::cout << "\nError in addValueInNewDb when try to get values from table " + table.toStdString() + ". " << selectQuery.lastError().text().toStdString() << std::endl;
		qDebug() << selectQuery.lastQuery();
		return;
	}

	selectQuery.next();

	if (!selectQuery.isValid())
	{
		qDebug() << " - Empty table";
		return;
	}
	else
	{
		// Выявляем столбцы с автоинкрементом и в случае выявления позволяем писать в столбцы с данным параметром

		if (!identitySelectQuery.exec((QString("SELECT name FROM [%1].sys.identity_columns WHERE OBJECT_NAME(object_id) = '%2'")).arg(mainDbName).arg(table)) || !identitySelectQuery.next())
		{
			if (identitySelectQuery.lastError().isValid())
			{
				std::cout << "\nError in addValueInNewDb when try to get all identity_columns in mainDB " + identitySelectQuery.lastError().text().toStdString() << std::endl;
				qDebug() << identitySelectQuery.lastQuery();
			}
		}
		else
		{
			if (identitySelectQuery.isValid())
			{
				identityInster = true;

				identityInsertString = QString("SET IDENTITY_INSERT [%1].[dbo].[%2] ON;")
					.arg(doppelDbName)
					.arg(table);
			}
		}

		// формируем запрос на внесение значений

		QString queryInsertString = QString("%1 INSERT INTO [%2].[dbo].[%3](")
			.arg(identityInsertString)
			.arg(doppelDbName)
			.arg(table);

		for (auto& val : any)
		{
			if (val.dataType == "timestamp")
				continue;
			queryInsertString += '[' + val.ColumnName + ']' + ','; // TEST
		}

		queryInsertString.chop(1);

		queryInsertString += ") VALUES(";


		// Определяем сколько всего записей в таблице

		selectQuery.last();
		long long countOfRowInQuery = selectQuery.at() + 1;
		selectQuery.first();

		int counter = 0; ////////////////////////////////////////////////////delete later !!

		// Интегрируем данные в новую таблицу

		do {

			// Формируем подготовленный запрос

			QString temporaryInsertForSingleString = queryInsertString;

			for (int counter = 0; counter < structArrayForTable.length(); counter++)
			{
				if (checkSpecialTypeArray[counter].contains("timestamp"))
					continue;

				temporaryInsertForSingleString += "?, ";
			}

			temporaryInsertForSingleString.chop(2);
			temporaryInsertForSingleString += ')';

			insertQuery.prepare(temporaryInsertForSingleString);

			// Наполняем запрос данными

			for (int counter = 0; counter < structArrayForTable.length(); counter++)
			{
				if (checkSpecialTypeArray[counter].contains("varbinary") || checkSpecialTypeArray[counter].contains("blob") || checkSpecialTypeArray[counter].contains("binary") || checkSpecialTypeArray[counter].contains("image") || checkSpecialTypeArray[counter].contains("timestamp"))
				{
					/*if (checkSpecialTypeArray[counter].contains("XML"))
					{
						QByteArray xmlBytes = selectQuery.value(counter).toByteArray();  // Получаем сырые байты

						QString xmlString;

						if (!xmlBytes.isEmpty()) {
							QTextCodec* codec = QTextCodec::codecForName("UTF-16LE");
							if (codec) {
								xmlString = codec->toUnicode(xmlBytes);
							}
						}
						insertQuery.addBindValue(xmlString.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : xmlString);

						continue;
					}*/

					if (checkSpecialTypeArray[counter].contains("timestamp"))
						continue;
					else
						insertQuery.addBindValue(selectQuery.value(counter).toByteArray(), QSql::In | QSql::Binary);
				}
				else
					insertQuery.addBindValue(selectQuery.value(counter).isNull() ? QVariant(QMetaType::fromType<QString>()) : selectQuery.value(counter).toString());
			}

			// Выполняем подготовленный запрос для внесение данных в таблицу

			if (!insertQuery.exec()) // подготовленный запрос выполняется без передачи строки в exec()
			{
				std::cout << "\n\nError in addValueInNewDb when try to insert values in table " + doppelDbName.toStdString() + ". " << insertQuery.lastError().text().toStdString() << std::endl;
				qDebug() << insertQuery.lastQuery();

				///////////////////////////////////////////
				QString tempQueryList;

				for (auto& val : insertQuery.boundValues())
				{
					tempQueryList += val.toString() + "   ";
				}

				qDebug() << tempQueryList << "\n";
				///////////////////////////////////////////
			}
			else
			{
				QString progressString = progress + " - Values was added into " + table + " [ " + QString::number(selectQuery.at() + 1) + " / " + QString::number(countOfRowInQuery) + " ] ";
				std::cout << "\r\x1b[2K" << progressString.toStdString() << std::flush; // делаем возврат корретки в текущей строке и затираем всю строку.
			}

			///////////////////////////////////////////////
			counter++;
			if (counter == 1) break;
			///////////////////////////////////////////////

		} while (selectQuery.next());

	}

	qDebug() << "";

	// ВЫключаем возможность записи в столбцы с автоинкрементом для данной таблицы

	if (identityInster)
	{
		identityInsertString = QString("SET IDENTITY_INSERT [%1].[dbo].[%2] OFF")
			.arg(doppelDbName)
			.arg(table);

		if (!insertQuery.exec(identityInsertString))
		{
			std::cout << "\nError in addValueInNewDb when try identity insert off " + table.toStdString() + ". " << insertQuery.lastError().text().toStdString() << std::endl;
			qDebug() << insertQuery.lastQuery();
		}
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
		//qDebug() << temporary;
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



void createFK()
{
	// Считываем FK из mainDB

	QSqlQuery checkAndCreateFKQuery(mainDb);
	QSqlQuery writeFkQuery(masterDb);

	QString queryString = QString(
		"SELECT"
		" OBJECT_NAME(fk.[parent_object_id]) AS PARENT,"
		" CASE WHEN fkOpt.is_not_trusted = 1 THEN 'WITH NOCHECK' ELSE 'WITH CHECK' END AS NOT_TRUSTED_CHECK,"
		" OBJECT_NAME(fk.[constraint_object_id]) AS CONSTR,"
		" sCol.name,"
		" OBJECT_NAME(fk.[referenced_object_id]) AS REFER,"

		" (SELECT name"
		" FROM [%1].[sys].[columns]"
		" WHERE column_id = fk.[referenced_column_id] AND object_id = fk.[referenced_object_id]) AS NAME_REF,"

		" CASE WHEN fkOpt.is_not_for_replication = 1 THEN 'NOT FOR REPLICATION' ELSE NULL END AS REPLICATION_CHECK,"
		" CASE WHEN fkOpt.delete_referential_action = 1 THEN 'CASCADE' ELSE 'NO ACTION' END AS DELETE_ACTION,"
		" CASE WHEN fkOpt.update_referential_action = 1 THEN 'CASCADE' ELSE 'NO ACTION' END AS UPDATE_ACTION"

		" FROM [%1].[sys].[foreign_key_columns] AS fk"

		" JOIN [%1].[sys].[columns] AS sCol"
		" ON fk.[parent_object_id] = sCol.object_id AND sCol.column_id = fk.[parent_column_id]"

		" JOIN [%1].[sys].[foreign_keys] AS fkOpt"
		" ON fkOpt.name = OBJECT_NAME(fk.[constraint_object_id])"

		" ORDER BY PARENT").arg(mainDbName);

	if (!checkAndCreateFKQuery.exec(queryString) || !checkAndCreateFKQuery.next())
	{
		if (checkAndCreateFKQuery.lastError().isValid())
		{
			std::cout << "Error in createFK when try to get all FK " + checkAndCreateFKQuery.lastError().text().toStdString() << std::endl;
			qDebug() << checkAndCreateFKQuery.lastQuery();
		}
	}
	else
	{
		// записываем FK в новую DB

		do {
			QString writeString = QString(
				"ALTER TABLE [%1].dbo.[%2] %3"
				" ADD CONSTRAINT %4"
				" FOREIGN KEY(%5)"
				" REFERENCES [%1].dbo.[%6] (%7)"
				"%8 ON DELETE %9"
				" ON UPDATE %10"
			)
				.arg(doppelDbName) // %1 name DB
				.arg(checkAndCreateFKQuery.value(0).toString()) // %2 PARENT TABLE
				.arg(checkAndCreateFKQuery.value(1).toString()) // %3 CHECK DATA
				.arg(checkAndCreateFKQuery.value(2).toString()) // %4 KEY NAME
				.arg(checkAndCreateFKQuery.value(3).toString()) // %5 PARENT COLUMN
				.arg(checkAndCreateFKQuery.value(4).toString()) // %6 REFERENCES TABLE
				.arg(checkAndCreateFKQuery.value(5).toString()) // %7 REFERENCES COLUMN
				.arg(checkAndCreateFKQuery.value(6).toString()) // %8 REPLICATION
				.arg(checkAndCreateFKQuery.value(7).toString()) // %8 HOW DELETE
				.arg(checkAndCreateFKQuery.value(8).toString()); // %8 HOW UPDATE

			if (!writeFkQuery.exec(writeString))
			{
				if (writeFkQuery.lastError().isValid())
				{
					std::cout << "Error in createFK when try add new FK in new DB " + writeFkQuery.lastError().text().toStdString() << std::endl;
					qDebug() << writeFkQuery.lastQuery();
				}
			}
			qDebug() << "ADD FK " << checkAndCreateFKQuery.value(2).toString(); //////////////////////////////////

		} while (checkAndCreateFKQuery.next());
	}
}



void createIndexInNewTable(QString tempTable)
{
	// Проставляем индексы в таблицы

	QSqlQuery getAllIndex(mainDb);
	QSqlQuery getIndexComponent(mainDb);
	QSqlQuery writeIndexInDoppelDb(masterDb);

	QString FullQueryForCreateIndex;

	// Получаем список всех индексов таблицы если таковые имеются

	QString queryString = QString(
		"SELECT"
		" [object_id]"
		", [name]"
		", [index_id]"
		", CASE WHEN[is_unique] = 1 THEN 'UNIQUE' ELSE '' END AS 'is_unique'"
		", [type_desc]"
		", [data_space_id]"
		", [ignore_dup_key]"
		", [is_primary_key]"
		", [is_unique_constraint]"
		", [fill_factor]"
		", [is_padded]"
		", [is_disabled]"
		", [is_hypothetical]"
		", [allow_row_locks]"
		", [allow_page_locks]"
		", [has_filter]"
		", [filter_definition]"
		", [compression_delay]"
		" FROM [%1].[sys].[indexes]"
		" WHERE OBJECT_NAME([object_id]) = '%2' AND is_primary_key != 1 AND index_id > 0 AND type > 0"
	)
		.arg(mainDbName)
		.arg(tempTable);

	if (!getAllIndex.exec(queryString) || !getAllIndex.next())
	{
		if (getAllIndex.lastError().isValid())
		{
			std::cout << " Error in createIndexInNewTable when try to get all index for temp table " + getAllIndex.lastError().text().toStdString() << std::endl;
			qDebug() << getAllIndex.lastQuery();
		}
		else
		{
			std::cout << ". Table " + tempTable.toStdString() + " havent index's or unknown ploblem";
			return;
		}
	}
	else
	{
		do
		{
			// Формируем список INDEX компонентнов из которых состоят индексы

			FullQueryForCreateIndex += "CREATE " + getAllIndex.value(3).toString() + " " + getAllIndex.value(4).toString() + " INDEX " + getAllIndex.value(1).toString() + " ON " + '[' + doppelDbName + "].dbo." + tempTable + " (";

			QString queryStringForComponent = QString(
				"SELECT"
				" inCol.[object_id],"
				" OBJECT_NAME(inCol.[object_id]) AS OBJNAME,"
				" inCol.[index_id],"
				" inCol.[index_column_id],"
				" inCol.[column_id],"
				" col.[name] AS NameCol,"
				" inCol.[key_ordinal],"
				" inCol.[partition_ordinal],"
				" CASE WHEN [is_descending_key] = 0 THEN 'ASC' ELSE 'DESC' END AS 'is_descending_key',"
				" inCol.[is_included_column]"
				" FROM[%1].[sys].[index_columns] AS inCol"
				" JOIN[%1].[sys].[columns] AS col"
				" ON inCol.[object_id] = col.[object_id]"
				" AND inCol.[column_id] = col.[column_id]"
				" WHERE inCol.[object_id] = '%2' AND index_id = '%3' AND is_included_column = '0'"
				" ORDER BY index_column_id"
			)
				.arg(mainDbName)
				.arg(getAllIndex.value(0).toString())
				.arg(getAllIndex.value(2).toString());

			if (!getIndexComponent.exec(queryStringForComponent) || !getIndexComponent.next())
			{
				if (getIndexComponent.lastError().isValid())
				{
					std::cout << " Error in createIndexInNewTable when try to get index component for temp table " + getAllIndex.lastError().text().toStdString() << std::endl;
					qDebug() << getAllIndex.lastQuery();
				}
				else
					qDebug() << " Index " + getAllIndex.value(1).toString() + " havent index component or unknown ploblem";
			}
			else
			{
				do
				{
					FullQueryForCreateIndex += getIndexComponent.value(5).toString() + ' ' + getIndexComponent.value(8).toString() + ", ";

				} while (getIndexComponent.next());

				FullQueryForCreateIndex.chop(2);
				FullQueryForCreateIndex += ')';
			}

			// Формируем список INCLUDE компонентнов из которых состоят индексы

			 queryStringForComponent = QString(
				"SELECT"
				" inCol.[object_id],"
				" OBJECT_NAME(inCol.[object_id]) AS OBJNAME,"
				" inCol.[index_id],"
				" inCol.[index_column_id],"
				" inCol.[column_id],"
				" col.[name] AS NameCol,"
				" inCol.[key_ordinal],"
				" inCol.[partition_ordinal],"
				" CASE WHEN [is_descending_key] = 0 THEN 'ASC' ELSE 'DESC' END AS 'is_descending_key',"
				" inCol.[is_included_column]"
				" FROM[%1].[sys].[index_columns] AS inCol"
				" JOIN[%1].[sys].[columns] AS col"
				" ON inCol.[object_id] = col.[object_id]"
				" AND inCol.[column_id] = col.[column_id]"
				" WHERE inCol.[object_id] = '%2' AND index_id = '%3' AND is_included_column = '1'"
				" ORDER BY index_column_id"
			)
				.arg(mainDbName)
				.arg(getAllIndex.value(0).toString())
				.arg(getAllIndex.value(2).toString());

			if (!getIndexComponent.exec(queryStringForComponent) || !getIndexComponent.next())
			{
				if (getIndexComponent.lastError().isValid())
				{
					std::cout << " Error in createIndexInNewTable when try to get include component for temp table " + getAllIndex.lastError().text().toStdString() << std::endl;
					qDebug() << getAllIndex.lastQuery();
				}
				else
					qDebug() << " Index " + getAllIndex.value(1).toString() + " havent include component or unknown ploblem";
			}
			else
			{
				FullQueryForCreateIndex += "INCLUDE (";

				do
				{
					FullQueryForCreateIndex += getIndexComponent.value(5).toString() + ' ' + getIndexComponent.value(8).toString() + ", ";

				} while (getIndexComponent.next());

				FullQueryForCreateIndex.chop(2);
				FullQueryForCreateIndex += ')';
			}

			// Производим запись компонента из сформированной строки запроса

			if (!writeIndexInDoppelDb.exec(FullQueryForCreateIndex))
			{
				if (getIndexComponent.lastError().isValid())
				{
					std::cout << " Error in createIndexInNewTable when try to write new index in doppelDb " + writeIndexInDoppelDb.lastError().text().toStdString() << std::endl;
					qDebug() << writeIndexInDoppelDb.lastQuery();
				}
				else
					qDebug() << " Unknown ploblem when try to write new index in doppelDb";
				qDebug() << writeIndexInDoppelDb.lastQuery();
			}
			else
			{
				qDebug() << " Index " + getAllIndex.value(1).toString() + " was added";
			}

			FullQueryForCreateIndex.clear();

		} while (getAllIndex.next());
	}
}