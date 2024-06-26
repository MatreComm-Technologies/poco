//
// Unicode.cpp
//
// Library: Data/ODBC
// Package: ODBC
// Module:  Unicode
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/Data/ODBC/ODBC.h"
#include "Poco/Data/ODBC/Unicode_UNIXODBC.h"
#include "Poco/Data/ODBC/Utility.h"
#include "Poco/TextConverter.h"
#include "Poco/UTF8Encoding.h"
#include "Poco/UTF16Encoding.h"
#include "Poco/Buffer.h"
#include "Poco/Exception.h"
#include <iostream>


using Poco::Buffer;
using Poco::UTF8Encoding;
using Poco::UTF16Encoding;
using Poco::TextConverter;
using Poco::InvalidArgumentException;
using Poco::NotImplementedException;


namespace Poco {
namespace Data {
namespace ODBC {


void makeUTF16(SQLCHAR* pSQLChar, SQLINTEGER length, std::string& target)
{
	int len = length;
	if (SQL_NTS == len)
		len = (int) std::strlen((const char *) pSQLChar);

	UTF8Encoding utf8Encoding;
	UTF16Encoding utf16Encoding;
	TextConverter converter(utf8Encoding, utf16Encoding);

	if (0 != converter.convert(pSQLChar, len, target))
		throw DataFormatException("Error converting UTF-8 to UTF-16");
}


void makeUTF8(Poco::Buffer<SQLWCHAR>& buffer, SQLINTEGER length, SQLPOINTER pTarget, SQLINTEGER targetLength)
{
	UTF8Encoding utf8Encoding;
	UTF16Encoding utf16Encoding;
	TextConverter converter(utf16Encoding, utf8Encoding);

	std::string result;
	if (0 != converter.convert(buffer.begin(), length, result))
		throw DataFormatException("Error converting UTF-16 to UTF-8");

	std::memset(pTarget, 0, targetLength);
	std::strncpy((char*) pTarget, result.c_str(), result.size() < targetLength ? result.size() : targetLength);
}


SQLRETURN SQLColAttribute(SQLHSTMT hstmt,
	SQLUSMALLINT   iCol,
	SQLUSMALLINT   iField,
	SQLPOINTER	   pCharAttr,
	SQLSMALLINT	   cbCharAttrMax,
	SQLSMALLINT*   pcbCharAttr,
	NumAttrPtrType pNumAttr)
{
	SQLSMALLINT cbCharAttr = 0;
	if (!pcbCharAttr) pcbCharAttr = &cbCharAttr;

	SQLSMALLINT cbCharAttr;
	if (!pcbCharAttr) pcbCharAttr = &cbCharAttr;

	if (isString(pCharAttr, cbCharAttrMax))
	{
		Buffer<SQLWCHAR> buffer(stringLength(pCharAttr, cbCharAttrMax));

		SQLRETURN rc = SQLColAttributeW(hstmt,
			iCol,
			iField,
			buffer.begin(),
			(SQLSMALLINT) buffer.sizeBytes(),
			pcbCharAttr,
			pNumAttr);

		if (!Utility::isError(rc))
			makeUTF8(buffer, *pcbCharAttr, pCharAttr, cbCharAttrMax);

		return rc;
	}

	return SQLColAttributeW(hstmt,
		iCol,
		iField,
		pCharAttr,
		cbCharAttrMax,
		pcbCharAttr,
		pNumAttr);
}


SQLRETURN SQLColAttributes(SQLHSTMT hstmt,
	SQLUSMALLINT icol,
	SQLUSMALLINT fDescType,
	SQLPOINTER   rgbDesc,
	SQLSMALLINT  cbDescMax,
	SQLSMALLINT* pcbDesc,
	SQLLEN*      pfDesc)
{
	SQLSMALLINT cbDesc = 0;
	if (!pcbDesc) pcbDesc = &cbDesc;
	SQLLEN fDesc = 0;
	if (!pfDesc) pfDesc = &fDesc;

	return SQLColAttribute(hstmt,
		icol,
		fDescType,
		rgbDesc,
		cbDescMax,
		pcbDesc,
		pfDesc);
}


SQLRETURN SQLConnect(SQLHDBC hdbc,
	SQLCHAR*    szDSN,
	SQLSMALLINT cbDSN,
	SQLCHAR*    szUID,
	SQLSMALLINT cbUID,
	SQLCHAR*    szAuthStr,
	SQLSMALLINT cbAuthStr)
{
	std::string sqlDSN;
	makeUTF16(szDSN, cbDSN, sqlDSN);

	std::string sqlUID;
	makeUTF16(szUID, cbUID, sqlUID);

	std::string sqlPWD;
	makeUTF16(szAuthStr, cbAuthStr, sqlPWD);

	return SQLConnectW(hdbc,
		(SQLWCHAR*) sqlDSN.c_str(), cbDSN,
		(SQLWCHAR*) sqlUID.c_str(), cbUID,
		(SQLWCHAR*) sqlPWD.c_str(), cbAuthStr);
}


SQLRETURN SQLDescribeCol(SQLHSTMT hstmt,
	SQLUSMALLINT icol,
	SQLCHAR*     szColName,
	SQLSMALLINT  cbColNameMax,
	SQLSMALLINT* pcbColName,
	SQLSMALLINT* pfSqlType,
	SQLULEN*     pcbColDef,
	SQLSMALLINT* pibScale,
	SQLSMALLINT* pfNullable)
{
	SQLSMALLINT cbColName = 0;
	if (!pcbColName) pcbColName = &cbColName;
	SQLSMALLINT fSqlType = 0;
	if (!pfSqlType) pfSqlType = &fSqlType;
	SQLULEN cbColDef = 0;
	if (!pcbColDef) pcbColDef = &cbColDef;
	SQLSMALLINT ibScale = 0;
	if (!pibScale) pibScale = &ibScale;
	SQLSMALLINT fNullable = 0;
	if (!pfNullable) pfNullable = &fNullable;

	Buffer<SQLWCHAR> buffer(cbColNameMax);
	SQLRETURN rc = SQLDescribeColW(hstmt,
		icol,
		(SQLWCHAR*) buffer.begin(),
		(SQLSMALLINT) buffer.size(),
		pcbColName,
		pfSqlType,
		pcbColDef,
		pibScale,
		pfNullable);

	if (!Utility::isError(rc))
		makeUTF8(buffer, *pcbColName * sizeof(SQLWCHAR), szColName, cbColNameMax);

	return rc;
}


SQLRETURN SQLError(SQLHENV henv,
	SQLHDBC      hdbc,
	SQLHSTMT     hstmt,
	SQLCHAR*     szSqlState,
	SQLINTEGER*  pfNativeError,
	SQLCHAR*     szErrorMsg,
	SQLSMALLINT  cbErrorMsgMax,
	SQLSMALLINT* pcbErrorMsg)
{
	throw NotImplementedException("SQLError is obsolete. "
		"Use SQLGetDiagRec instead.");
}


SQLRETURN SQLExecDirect(SQLHSTMT hstmt,
	SQLCHAR*   szSqlStr,
	SQLINTEGER cbSqlStr)
{
	std::string sqlStr;
	makeUTF16(szSqlStr, cbSqlStr, sqlStr);

	return SQLExecDirectW(hstmt, (SQLWCHAR*) sqlStr.c_str(), cbSqlStr);
}


SQLRETURN SQLGetConnectAttr(SQLHDBC hdbc,
	SQLINTEGER  fAttribute,
	SQLPOINTER  rgbValue,
	SQLINTEGER  cbValueMax,
	SQLINTEGER* pcbValue)
{
	SQLINTEGER cbValue = 0;
	if (!pcbValue) pcbValue = &cbValue;

	if (isString(rgbValue, cbValueMax))
	{
		Buffer<SQLWCHAR> buffer(stringLength(rgbValue, cbValueMax));

		SQLRETURN rc = SQLGetConnectAttrW(hdbc,
				fAttribute,
				buffer.begin(),
				(SQLINTEGER) buffer.sizeBytes(),
				pcbValue);

		if (!Utility::isError(rc))
			makeUTF8(buffer, *pcbValue, rgbValue, cbValueMax);
		return rc;
	}


	return SQLGetConnectAttrW(hdbc,
		fAttribute,
		rgbValue,
		cbValueMax,
		pcbValue);
}


SQLRETURN SQLGetCursorName(SQLHSTMT hstmt,
	SQLCHAR*     szCursor,
	SQLSMALLINT  cbCursorMax,
	SQLSMALLINT* pcbCursor)
{
	throw NotImplementedException("Not implemented");
}


SQLRETURN SQLSetDescField(SQLHDESC hdesc,
	SQLSMALLINT iRecord,
	SQLSMALLINT iField,
	SQLPOINTER  rgbValue,
	SQLINTEGER  cbValueMax)
{
	if (isString(rgbValue, cbValueMax))
	{
		std::string str;
		makeUTF16((SQLCHAR*) rgbValue, cbValueMax, str);

		return SQLSetDescFieldW(hdesc,
			iRecord,
			iField,
			(SQLPOINTER) str.c_str(),
			(SQLINTEGER) str.size() * sizeof(SQLWCHAR));
	}

	return SQLSetDescFieldW(hdesc,
		iRecord,
		iField,
		rgbValue,
		cbValueMax);
}


SQLRETURN SQLGetDescField(SQLHDESC hdesc,
	SQLSMALLINT iRecord,
	SQLSMALLINT iField,
	SQLPOINTER  rgbValue,
	SQLINTEGER	cbValueMax,
	SQLINTEGER* pcbValue)
{
	SQLINTEGER cbValue = 0;
	if (!pcbValue) pcbValue = &cbValue;

	if (isString(rgbValue, cbValueMax))
	{
		Buffer<SQLWCHAR> buffer(stringLength(rgbValue, cbValueMax));

		SQLRETURN rc = SQLGetDescFieldW(hdesc,
			iRecord,
			iField,
			buffer.begin(),
			(SQLINTEGER) buffer.sizeBytes(),
			pcbValue);

		if (!Utility::isError(rc))
			makeUTF8(buffer, *pcbValue, rgbValue, cbValueMax);

		return rc;
	}

	return SQLGetDescFieldW(hdesc,
		iRecord,
		iField,
		rgbValue,
		cbValueMax,
		pcbValue);
}


SQLRETURN SQLGetDescRec(SQLHDESC hdesc,
	SQLSMALLINT  iRecord,
	SQLCHAR*     szName,
	SQLSMALLINT  cbNameMax,
	SQLSMALLINT* pcbName,
	SQLSMALLINT* pfType,
	SQLSMALLINT* pfSubType,
	SQLLEN*      pLength,
	SQLSMALLINT* pPrecision,
	SQLSMALLINT* pScale,
	SQLSMALLINT* pNullable)
{
	throw NotImplementedException();
}


SQLRETURN SQLGetDiagField(SQLSMALLINT fHandleType,
	SQLHANDLE    handle,
	SQLSMALLINT  iRecord,
	SQLSMALLINT  fDiagField,
	SQLPOINTER   rgbDiagInfo,
	SQLSMALLINT  cbDiagInfoMax,
	SQLSMALLINT* pcbDiagInfo)
{
	SQLSMALLINT cbDiagInfo = 0;
	if (!pcbDiagInfo) pcbDiagInfo = &cbDiagInfo;

	if (isString(rgbDiagInfo, cbDiagInfoMax))
	{
		Buffer<SQLWCHAR> buffer(stringLength(rgbDiagInfo, cbDiagInfoMax));

		SQLRETURN rc = SQLGetDiagFieldW(fHandleType,
			handle,
			iRecord,
			fDiagField,
			buffer.begin(),
			(SQLSMALLINT) buffer.sizeBytes(),
			pcbDiagInfo);

		if (!Utility::isError(rc))
			makeUTF8(buffer, *pcbDiagInfo, rgbDiagInfo, cbDiagInfoMax);

		return rc;
	}

	return SQLGetDiagFieldW(fHandleType,
		handle,
		iRecord,
		fDiagField,
		rgbDiagInfo,
		cbDiagInfoMax,
		pcbDiagInfo);
}


SQLRETURN SQLGetDiagRec(SQLSMALLINT fHandleType,
	SQLHANDLE    handle,
	SQLSMALLINT  iRecord,
	SQLCHAR*     szSqlState,
	SQLINTEGER*  pfNativeError,
	SQLCHAR*     szErrorMsg,
	SQLSMALLINT  cbErrorMsgMax,
	SQLSMALLINT* pcbErrorMsg)
{
	SQLINTEGER fNativeError = 0;
	if (!pfNativeError) pfNativeError = &fNativeError;
	SQLSMALLINT cbErrorMsg = 0;
	if (!pcbErrorMsg) pcbErrorMsg = &cbErrorMsg;

	const SQLINTEGER stateLen = SQL_SQLSTATE_SIZE + 1;
	Buffer<SQLWCHAR> bufState(stateLen);
	Buffer<SQLWCHAR> bufErr(cbErrorMsgMax);

	SQLRETURN rc = SQLGetDiagRecW(fHandleType,
		handle,
		iRecord,
		bufState.begin(),
		pfNativeError,
		bufErr.begin(),
		(SQLSMALLINT) bufErr.size(),
		pcbErrorMsg);

	if (!Utility::isError(rc))
	{
		makeUTF8(bufState, stateLen * sizeof(SQLWCHAR), szSqlState, stateLen);
		makeUTF8(bufErr, *pcbErrorMsg * sizeof(SQLWCHAR), szErrorMsg, cbErrorMsgMax);
	}

	return rc;
}


SQLRETURN SQLPrepare(SQLHSTMT hstmt,
	SQLCHAR*   szSqlStr,
	SQLINTEGER cbSqlStr)
{
	std::string sqlStr;
	makeUTF16(szSqlStr, cbSqlStr, sqlStr);

	return SQLPrepareW(hstmt, (SQLWCHAR*) sqlStr.c_str(), (SQLINTEGER) sqlStr.size());
}


SQLRETURN SQLSetConnectAttr(SQLHDBC hdbc,
	SQLINTEGER fAttribute,
	SQLPOINTER rgbValue,
	SQLINTEGER cbValue)
{
	if (isString(rgbValue, cbValue))
	{
		std::string str;
		makeUTF16((SQLCHAR*) rgbValue, cbValue, str);

		return SQLSetConnectAttrW(hdbc,
			fAttribute,
			(SQLWCHAR*) str.c_str(),
			(SQLINTEGER) str.size() * sizeof(SQLWCHAR));
	}

	return SQLSetConnectAttrW(hdbc,	fAttribute,	rgbValue, cbValue);
}


SQLRETURN SQLSetCursorName(SQLHSTMT hstmt,
	SQLCHAR*    szCursor,
	SQLSMALLINT cbCursor)
{
	throw NotImplementedException("Not implemented");
}


SQLRETURN SQLSetStmtAttr(SQLHSTMT hstmt,
	SQLINTEGER fAttribute,
	SQLPOINTER rgbValue,
	SQLINTEGER cbValueMax)
{
	if (isString(rgbValue, cbValueMax))
	{
		std::string str;
		makeUTF16((SQLCHAR*) rgbValue, cbValueMax, str);

		return SQLSetStmtAttrW(hstmt,
			fAttribute,
			(SQLWCHAR*) str.c_str(),
			(SQLINTEGER) str.size());
	}

	return SQLSetStmtAttrW(hstmt, fAttribute, rgbValue,	cbValueMax);
}


SQLRETURN SQLGetStmtAttr(SQLHSTMT hstmt,
	SQLINTEGER  fAttribute,
	SQLPOINTER  rgbValue,
	SQLINTEGER  cbValueMax,
	SQLINTEGER* pcbValue)
{
	SQLINTEGER cbValue = 0;
	if (!pcbValue) pcbValue = &cbValue;

	if (isString(rgbValue, cbValueMax))
	{
		Buffer<SQLWCHAR> buffer(stringLength(rgbValue, cbValueMax));

		return SQLGetStmtAttrW(hstmt,
			fAttribute,
			(SQLPOINTER) buffer.begin(),
			(SQLINTEGER) buffer.sizeBytes(),
			pcbValue);
	}

	return SQLGetStmtAttrW(hstmt, fAttribute, rgbValue, cbValueMax, pcbValue);
}


SQLRETURN SQLColumns(SQLHSTMT hstmt,
	SQLCHAR*    szCatalogName,
	SQLSMALLINT cbCatalogName,
	SQLCHAR*    szSchemaName,
	SQLSMALLINT cbSchemaName,
	SQLCHAR*    szTableName,
	SQLSMALLINT cbTableName,
	SQLCHAR*    szColumnName,
	SQLSMALLINT cbColumnName)
{
	throw NotImplementedException();
}


SQLRETURN SQLGetConnectOption(SQLHDBC hdbc,
	SQLUSMALLINT fOption,
	SQLPOINTER   pvParam)
{
	throw NotImplementedException();
}


SQLRETURN SQLGetInfo(SQLHDBC hdbc,
	SQLUSMALLINT fInfoType,
	SQLPOINTER   rgbInfoValue,
	SQLSMALLINT  cbInfoValueMax,
	SQLSMALLINT* pcbInfoValue)
{
	SQLSMALLINT cbInfoValue = 0;
	if (!pcbInfoValue) pcbInfoValue = &cbInfoValue;

	if (cbInfoValueMax)
	{
		Buffer<SQLWCHAR> buffer(cbInfoValueMax);

		SQLRETURN rc = SQLGetInfoW(hdbc,
			fInfoType,
			(SQLPOINTER) buffer.begin(),
			(SQLSMALLINT) buffer.sizeBytes(),
			pcbInfoValue);

		if (!Utility::isError(rc))
			makeUTF8(buffer, *pcbInfoValue, rgbInfoValue, cbInfoValueMax);

		return rc;
	}

	return SQLGetInfoW(hdbc, fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue);
}


SQLRETURN SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
	return SQLGetTypeInfoW(StatementHandle, DataType);
}


SQLRETURN SQLSetConnectOption(SQLHDBC hdbc,
	SQLUSMALLINT fOption,
	SQLULEN      vParam)
{
	throw NotImplementedException();
}


SQLRETURN SQLSpecialColumns(SQLHSTMT hstmt,
	SQLUSMALLINT fColType,
	SQLCHAR*     szCatalogName,
	SQLSMALLINT  cbCatalogName,
	SQLCHAR*     szSchemaName,
	SQLSMALLINT  cbSchemaName,
	SQLCHAR*     szTableName,
	SQLSMALLINT  cbTableName,
	SQLUSMALLINT fScope,
	SQLUSMALLINT fNullable)
{
	throw NotImplementedException();
}


SQLRETURN SQLStatistics(SQLHSTMT hstmt,
	SQLCHAR*     szCatalogName,
	SQLSMALLINT  cbCatalogName,
	SQLCHAR*     szSchemaName,
	SQLSMALLINT  cbSchemaName,
	SQLCHAR*     szTableName,
	SQLSMALLINT  cbTableName,
	SQLUSMALLINT fUnique,
	SQLUSMALLINT fAccuracy)
{
	throw NotImplementedException();
}


SQLRETURN SQLTables(SQLHSTMT hstmt,
	SQLCHAR*    szCatalogName,
	SQLSMALLINT cbCatalogName,
	SQLCHAR*    szSchemaName,
	SQLSMALLINT cbSchemaName,
	SQLCHAR*    szTableName,
	SQLSMALLINT cbTableName,
	SQLCHAR*    szTableType,
	SQLSMALLINT cbTableType)
{
	throw NotImplementedException();
}


SQLRETURN SQLDataSources(SQLHENV henv,
	SQLUSMALLINT fDirection,
	SQLCHAR*     szDSN,
	SQLSMALLINT  cbDSNMax,
	SQLSMALLINT* pcbDSN,
	SQLCHAR*     szDesc,
	SQLSMALLINT  cbDescMax,
	SQLSMALLINT* pcbDesc)
{
	SQLSMALLINT cbDSN = 0, cbDesc = 0;
	if (!pcbDSN) pcbDSN = &cbDSN;
	if (!pcbDesc) pcbDesc = &cbDesc;

	Buffer<SQLWCHAR> bufDSN(cbDSNMax);
	Buffer<SQLWCHAR> bufDesc(cbDescMax);

	SQLRETURN rc = SQLDataSourcesW(henv,
		fDirection,
		bufDSN.begin(),
		(SQLSMALLINT) bufDSN.size(),
		pcbDSN,
		bufDesc.begin(),
		(SQLSMALLINT) bufDesc.size(),
		pcbDesc);

	if (!Utility::isError(rc))
	{
		makeUTF8(bufDSN, *pcbDSN * sizeof(SQLWCHAR), szDSN, cbDSNMax);
		makeUTF8(bufDesc, *pcbDesc * sizeof(SQLWCHAR), szDesc, cbDescMax);
	}

	return rc;
}


SQLRETURN SQLDriverConnect(SQLHDBC hdbc,
	SQLHWND      hwnd,
	SQLCHAR*     szConnStrIn,
	SQLSMALLINT  cbConnStrIn,
	SQLCHAR*     szConnStrOut,
	SQLSMALLINT  cbConnStrOutMax,
	SQLSMALLINT* pcbConnStrOut,
	SQLUSMALLINT fDriverCompletion)
{
	SQLSMALLINT cbConnStrOut = 0;
	if (!pcbConnStrOut) pcbConnStrOut = &cbConnStrOut;

	SQLSMALLINT len = cbConnStrIn;
	if (SQL_NTS == len)
		len = (SQLSMALLINT) std::strlen((const char*) szConnStrIn) + 1;

	std::string connStrIn;
	makeUTF16(szConnStrIn, len, connStrIn);

	Buffer<SQLWCHAR> out(cbConnStrOutMax);
	SQLRETURN rc = SQLDriverConnectW(hdbc,
		hwnd,
		(SQLWCHAR*) connStrIn.c_str(),
		(SQLSMALLINT) connStrIn.size(),
		out.begin(),
		cbConnStrOutMax,
		pcbConnStrOut,
		fDriverCompletion);

	if (!Utility::isError(rc))
		makeUTF8(out, *pcbConnStrOut * sizeof(SQLWCHAR), pcbConnStrOut, cbConnStrOutMax);

	return rc;
}


SQLRETURN SQLBrowseConnect(SQLHDBC hdbc,
	SQLCHAR*     szConnStrIn,
	SQLSMALLINT  cbConnStrIn,
	SQLCHAR*     szConnStrOut,
	SQLSMALLINT  cbConnStrOutMax,
	SQLSMALLINT* pcbConnStrOut)
{
	SQLSMALLINT cbConnStrOut = 0;
	if (!pcbConnStrOut) pcbConnStrOut = &cbConnStrOut;

	std::string str;
	makeUTF16(szConnStrIn, cbConnStrIn, str);

	Buffer<SQLWCHAR> bufConnStrOut(cbConnStrOutMax);

	SQLRETURN rc = SQLBrowseConnectW(hdbc,
		(SQLWCHAR*) str.c_str(),
		(SQLSMALLINT) str.size(),
		bufConnStrOut.begin(),
		(SQLSMALLINT) bufConnStrOut.size(),
		pcbConnStrOut);

	if (!Utility::isError(rc))
		makeUTF8(bufConnStrOut, *pcbConnStrOut * sizeof(SQLWCHAR), szConnStrOut, cbConnStrOutMax);

	return rc;
}


SQLRETURN SQLColumnPrivileges(SQLHSTMT hstmt,
	SQLCHAR*    szCatalogName,
	SQLSMALLINT cbCatalogName,
	SQLCHAR*    szSchemaName,
	SQLSMALLINT cbSchemaName,
	SQLCHAR*    szTableName,
	SQLSMALLINT cbTableName,
	SQLCHAR*    szColumnName,
	SQLSMALLINT cbColumnName)
{
	throw NotImplementedException();
}


SQLRETURN SQLForeignKeys(SQLHSTMT hstmt,
	SQLCHAR*    szPkCatalogName,
	SQLSMALLINT cbPkCatalogName,
	SQLCHAR*    szPkSchemaName,
	SQLSMALLINT cbPkSchemaName,
	SQLCHAR*    szPkTableName,
	SQLSMALLINT cbPkTableName,
	SQLCHAR*    szFkCatalogName,
	SQLSMALLINT cbFkCatalogName,
	SQLCHAR*    szFkSchemaName,
	SQLSMALLINT cbFkSchemaName,
	SQLCHAR*    szFkTableName,
	SQLSMALLINT cbFkTableName)
{
	throw NotImplementedException();
}


SQLRETURN SQLNativeSql(SQLHDBC hdbc,
	SQLCHAR*    szSqlStrIn,
	SQLINTEGER  cbSqlStrIn,
	SQLCHAR*    szSqlStr,
	SQLINTEGER  cbSqlStrMax,
	SQLINTEGER* pcbSqlStr)
{
	SQLINTEGER cbSqlStr = 0;
	if (!pcbSqlStr) pcbSqlStr = &cbSqlStr;

	std::string str;
	makeUTF16(szSqlStrIn, cbSqlStrIn, str);

	Buffer<SQLWCHAR> bufSQLOut(cbSqlStrMax);

	SQLRETURN rc = SQLNativeSqlW(hdbc,
		(SQLWCHAR*) str.c_str(),
		(SQLINTEGER) str.size(),
		bufSQLOut.begin(),
		(SQLINTEGER) bufSQLOut.size(),
		pcbSqlStr);

	if (!Utility::isError(rc))
		makeUTF8(bufSQLOut, *pcbSqlStr * sizeof(SQLWCHAR), szSqlStr, cbSqlStrMax);

	return rc;
}


SQLRETURN SQLPrimaryKeys(SQLHSTMT hstmt,
	SQLCHAR*    szCatalogName,
	SQLSMALLINT cbCatalogName,
	SQLCHAR*    szSchemaName,
	SQLSMALLINT cbSchemaName,
	SQLCHAR*    szTableName,
	SQLSMALLINT cbTableName)
{
	throw NotImplementedException();
}


SQLRETURN SQLProcedureColumns(SQLHSTMT hstmt,
	SQLCHAR*    szCatalogName,
	SQLSMALLINT cbCatalogName,
	SQLCHAR*    szSchemaName,
	SQLSMALLINT cbSchemaName,
	SQLCHAR*    szProcName,
	SQLSMALLINT cbProcName,
	SQLCHAR*    szColumnName,
	SQLSMALLINT cbColumnName)
{
	throw NotImplementedException();
}


SQLRETURN SQLProcedures(SQLHSTMT hstmt,
	SQLCHAR*    szCatalogName,
	SQLSMALLINT cbCatalogName,
	SQLCHAR*    szSchemaName,
	SQLSMALLINT cbSchemaName,
	SQLCHAR*    szProcName,
	SQLSMALLINT cbProcName)
{
	throw NotImplementedException();
}


SQLRETURN SQLTablePrivileges(SQLHSTMT hstmt,
	SQLCHAR*    szCatalogName,
	SQLSMALLINT cbCatalogName,
	SQLCHAR*    szSchemaName,
	SQLSMALLINT cbSchemaName,
	SQLCHAR*    szTableName,
	SQLSMALLINT cbTableName)
{
	throw NotImplementedException();
}


SQLRETURN SQLDrivers(SQLHENV henv,
	SQLUSMALLINT fDirection,
	SQLCHAR*     szDriverDesc,
	SQLSMALLINT  cbDriverDescMax,
	SQLSMALLINT* pcbDriverDesc,
	SQLCHAR*     szDriverAttributes,
	SQLSMALLINT  cbDrvrAttrMax,
	SQLSMALLINT* pcbDrvrAttr)
{
	SQLSMALLINT cbDriverDesc = 0, cbDrvrAttr = 0;
	if (!pcbDriverDesc) pcbDriverDesc = &cbDriverDesc;
	if (!pcbDrvrAttr) pcbDrvrAttr = &cbDrvrAttr;

	Buffer<SQLWCHAR> bufDriverDesc(cbDriverDescMax);
	Buffer<SQLWCHAR> bufDriverAttr(cbDrvrAttrMax);

	SQLRETURN rc = SQLDriversW(henv,
		fDirection,
		bufDriverDesc.begin(),
		(SQLSMALLINT) bufDriverDesc.size(),
		pcbDriverDesc,
		bufDriverAttr.begin(),
		(SQLSMALLINT) bufDriverAttr.size(),
		pcbDrvrAttr);

	if (!Utility::isError(rc))
	{
		makeUTF8(bufDriverDesc, *pcbDriverDesc * sizeof(SQLWCHAR), szDriverDesc, cbDriverDescMax);
		makeUTF8(bufDriverAttr, *pcbDrvrAttr * sizeof(SQLWCHAR), szDriverAttributes, cbDrvrAttrMax);
	}

	return rc;
}


} } } // namespace Poco::Data::ODBC
