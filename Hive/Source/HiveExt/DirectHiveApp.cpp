/*
* Copyright (C) 2009-2012 Rajko Stojadinovic <http://github.com/rajkosto/hive>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "DirectHiveApp.h"

DirectHiveApp::DirectHiveApp(string suffixDir) : HiveExtApp(suffixDir) {}

#include "Shared/Library/Database/DatabaseLoader.h"
#include "HiveLib/DataSource/SqlCharDataSource.h"
#include "HiveLib/DataSource/SqlObjDataSource.h"
#include "HiveLib/DataSource/SqlCustDataSource.h"

bool DirectHiveApp::initialiseService()
{
	_charDb = DatabaseLoader::create(DatabaseLoader::DBTYPE_MYSQL);
	Poco::Logger& dbLogger = Poco::Logger::get("Database");

	string initString;
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> globalDBConf(config().createView("Database"));
		initString = DatabaseLoader::makeInitString(globalDBConf);
	}

	if (!_charDb->initialise(dbLogger,initString))
		return false;

	_charDb->allowAsyncOperations();
	_objDb = _charDb;
	_custDb = _charDb;
	
	//pass the db along to character datasource
	{
		static const string defaultID = "PlayerUID";
		static const string defaultWS = "Worldspace";

		Poco::AutoPtr<Poco::Util::AbstractConfiguration> charDBConf(config().createView("Characters"));
		_charData.reset(new SqlCharDataSource(logger(),_charDb,charDBConf->getString("IDField",defaultID),charDBConf->getString("WSField",defaultWS)));	
	}

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> objDBConf(config().createView("ObjectDB"));
	bool useExternalObjDb = objDBConf->getBool("Use",false);
	if (useExternalObjDb)
	{
		try 
		{ 
			_objDb = DatabaseLoader::create(objDBConf); 
		} 
		catch (const DatabaseLoader::CreationError&) 
		{ 
			return false; 
		}
		
		Poco::Logger& objDBLogger = Poco::Logger::get("ObjectDB");

		if (!_objDb->initialise(objDBLogger,DatabaseLoader::makeInitString(objDBConf)))
			return false;

		_objDb->allowAsyncOperations();
	}

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> custDBConf(config().createView("CustomDB"));
	bool useExternalCustDb = custDBConf->getBool("Use",false);
	if (useExternalCustDb)
	{
		try { _custDb = DatabaseLoader::create(custDBConf); } 
		catch (const DatabaseLoader::CreationError&) { return false; }
		
		Poco::Logger& custDBLogger = Poco::Logger::get("CustomDB");
		custDBLogger.setLevel("trace");

		if (!_custDb->initialise(custDBLogger,DatabaseLoader::makeInitString(custDBConf)))
			return false;

		_custDb->allowAsyncOperations();
	}
	
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> objConf(config().createView("Objects"));
	//Poco::AutoPtr<Poco::Util::AbstractConfiguration> custConf(config().createView("Custom"));
	_objData.reset(new SqlObjDataSource(dbLogger,_objDb,objConf.get()));
	//_custData.reset(new SqlCustDataSource(_logger,_custDb,custConf.get()));
	_custData.reset(new SqlCustDataSource(dbLogger,_custDb));
	
	return true;
}
