//
// Utility.cpp
//
// Library: Data/ODBC
// Package: ODBC
// Module:  Utility
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/Data/ODBC/Utility.h"
#include "Poco/Data/ODBC/Connector.h"
#include "Poco/Data/ODBC/Handle.h"
#include "Poco/Data/ODBC/ODBCException.h"
#include "Poco/Data/LOB.h"
#include "Poco/NumberFormatter.h"
#include "Poco/DateTime.h"
#include <cmath>


namespace Poco {
namespace Data {
namespace ODBC {


const TypeInfo Utility::_dataTypes;


Utility::DriverMap& Utility::drivers(Utility::DriverMap& driverMap)
{
	static const EnvironmentHandle henv;
	const int length = sizeof(SQLCHAR) * 512;

	SQLCHAR desc[length];
	std::memset(desc, 0, length);
	SQLSMALLINT len1 = length;
	SQLCHAR attr[length];
	std::memset(attr, 0, length);
	SQLSMALLINT len2 = length;
	RETCODE rc = 0;

	if (!Utility::isError(rc = SQLDrivers(henv,
		SQL_FETCH_FIRST,
		desc,
		length,
		&len1,
		attr,
		len2,
		&len2)))
	{
		do
		{
			driverMap.insert(DSNMap::value_type(std::string((char *) desc),
				std::string((char *) attr)));
			std::memset(desc, 0, length);
			std::memset(attr, 0, length);
			len2 = length;
		}while (!Utility::isError(rc = SQLDrivers(henv,
			SQL_FETCH_NEXT,
			desc,
			length,
			&len1,
			attr,
			len2,
			&len2)));
	}

	if (SQL_NO_DATA != rc)
		throw EnvironmentException(henv);

	return driverMap;
}


Utility::DSNMap& Utility::dataSources(Utility::DSNMap& dsnMap)
{
	static const EnvironmentHandle henv;
	const int length = sizeof(SQLCHAR) * 512;
	const int dsnLength = sizeof(SQLCHAR) * (SQL_MAX_DSN_LENGTH + 1);

	SQLCHAR dsn[dsnLength];
	std::memset(dsn, 0, dsnLength);
	SQLSMALLINT len1 = sizeof(SQLCHAR) * SQL_MAX_DSN_LENGTH;
	SQLCHAR desc[length];
	std::memset(desc, 0, length);
	SQLSMALLINT len2 = length;
	RETCODE rc = 0;

	while (!Utility::isError(rc = Poco::Data::ODBC::SQLDataSources(henv,
		SQL_FETCH_NEXT,
		dsn,
		SQL_MAX_DSN_LENGTH,
		&len1,
		desc,
		len2,
		&len2)))
	{
		dsnMap.insert(DSNMap::value_type(std::string((char *) dsn), std::string((char *) desc)));
		std::memset(dsn, 0, dsnLength);
		std::memset(desc, 0, length);
		len2 = length;
	}

	if (SQL_NO_DATA != rc)
		throw EnvironmentException(henv);

	return dsnMap;
}


void Utility::dateTimeSync(Poco::DateTime& dt, const SQL_TIMESTAMP_STRUCT& ts)
{
	double msec = ts.fraction/1000000;
	double usec = 1000 * (msec - std::floor(msec));

	dt.assign(ts.year,
		ts.month,
		ts.day,
		ts.hour,
		ts.minute,
		ts.second,
		(int) std::floor(msec),
		(int) std::floor(usec));
}


void Utility::dateSync(SQL_DATE_STRUCT& ds, const Date& d)
{
	ds.year = d.year();
	ds.month = d.month();
	ds.day = d.day();
}


void Utility::timeSync(SQL_TIME_STRUCT& ts, const Time& t)
{
	ts.hour = t.hour();
	ts.minute = t.minute();
	ts.second = t.second();
}


void Utility::dateTimeSync(SQL_TIMESTAMP_STRUCT& ts, const Poco::DateTime& dt)
{
	ts.year = dt.year();
	ts.month = dt.month();
	ts.day = dt.day();
	ts.hour = dt.hour();
	ts.minute = dt.minute();
	ts.second = dt.second();
	// Fraction support is limited to milliseconds due to MS SQL Server limitation
	// see http://support.microsoft.com/kb/263872
	ts.fraction = (dt.millisecond() * 1000000);// + (dt.microsecond() * 1000);
}

int Utility::cDataType(const std::type_info &type)
{
    if (type == typeid(std::string))
        return SQL_C_CHAR;
    else if (type == typeid(Poco::UTF16String))
        return SQL_C_WCHAR;
    else if (type == typeid(bool))
        return SQL_C_BIT;
    else if (type == typeid(char))
        return SQL_C_STINYINT;
    else if (type == typeid(Int8))
        return SQL_C_STINYINT;
    else if (type == typeid(UInt8))
        return SQL_C_UTINYINT;
    else if (type == typeid(Int16))
        return SQL_C_SSHORT;
    else if (type == typeid(UInt16))
        return SQL_C_USHORT;
    else if (type == typeid(Int32))
        return SQL_C_SLONG;
    else if (type == typeid(UInt32))
        return SQL_C_ULONG;
    else if (type == typeid(Int64))
        return SQL_C_SBIGINT;
    else if (type == typeid(UInt64))
        return SQL_C_UBIGINT;
    else if (type == typeid(float))
        return SQL_C_FLOAT;
    else if (type == typeid(double))
        return SQL_C_DOUBLE;
    else if (type == typeid(DateTime) || type == typeid(Timestamp))
        return SQL_C_TYPE_TIMESTAMP;
    else if (type == typeid(Date))
        return SQL_C_TYPE_DATE;
    else if (type == typeid(Time))
        return SQL_C_TYPE_TIME;
    else if (type == typeid(BLOB) || type == typeid(CLOB))
        return SQL_C_BINARY;
    else if (type == typeid(BLOB))
        return SQL_C_BINARY;
    else if (type == typeid(UUID))
        return SQL_C_BINARY;
#ifndef POCO_INT64_IS_LONG
    else if (type == typeid(long) || type == typeid(unsigned long))
        return SQL_C_SLONG;
#endif

	throw UnknownTypeException(std::string(type.name()));
}

int Utility::sqlDataType(int cDataType, const std::type_info &type)
{
    switch (cDataType) {
    case SQL_C_BINARY:
        return type == typeid(UUID) ? SQL_GUID : SQL_LONGVARBINARY;
    case SQL_C_CHAR:
        return Connector::stringBoundToLongVarChar() ? SQL_LONGVARCHAR : SQL_VARCHAR;
    case SQL_C_WCHAR:
        return SQL_WLONGVARCHAR;
    default:
        return Utility::sqlDataType(cDataType);
    }
}


} } } // namespace Poco::Data::ODBC
