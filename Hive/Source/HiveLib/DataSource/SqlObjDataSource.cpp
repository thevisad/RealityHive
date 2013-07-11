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

#include "SqlObjDataSource.h"
#include "Database/Database.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;

namespace
{
	typedef boost::optional<Sqf::Value> PositionInfo;
	class PositionFixerVisitor : public boost::static_visitor<PositionInfo>
	{
		int max_x, max_y;
	public:
		explicit PositionFixerVisitor(const int x, const int y) : max_x(x), max_y(y) {}
		PositionInfo operator()(Sqf::Parameters& pos, int max_x, int max_y) const 
		{ 
			if (pos.size() != 3)
				return PositionInfo();

			try
			{
				double x = Sqf::GetDouble(pos[0]);
				double y = Sqf::GetDouble(pos[1]);
				double z = Sqf::GetDouble(pos[2]);

				if (x < max_x || y > max_y)
				{
					PositionInfo fixed(pos);
					pos.clear();
					return fixed;
				}
			}
			catch(const boost::bad_get&) {}

			return PositionInfo();
		}
		template<typename T> PositionInfo operator()(const T& other) const	{ return PositionInfo(); }
	};

	class WorldspaceFixerVisitor : public boost::static_visitor<PositionInfo>
	{
		int max_x, max_y;
	public:
		explicit WorldspaceFixerVisitor(const int x, const int y) : max_x(x), max_y(y) {}
		PositionInfo operator()(Sqf::Parameters& ws) const 
		{
			if (ws.size() != 2)
				return PositionInfo();

			return boost::apply_visitor(PositionFixerVisitor(max_x, max_y),ws[1]);
		}
		template<typename T> PositionInfo operator()(const T& other) const	{ return PositionInfo(); }
	};

	PositionInfo FixOOBWorldspace(Sqf::Value& v, const int max_x, const int max_y) { return boost::apply_visitor(WorldspaceFixerVisitor(max_x, max_y),v); }
};

#include <Poco/Util/AbstractConfiguration.h>
SqlObjDataSource::SqlObjDataSource( Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf ) : SqlDataSource(logger,db)
{
	_depTableName = getDB()->escape(conf->getString("Table","instance_deployable"));
	_vehTableName = getDB()->escape(conf->getString("Table","instance_vehicle"));
	//_cleanupPlacedDays = conf->getInt("CleanupPlacedAfterDays",6);
	_objectOOBReset = conf->getBool("ResetOOBObjects",false);
	//_vehicleOOBReset = conf->getBool("ResetOOBVehicles",false);
}

void SqlObjDataSource::populateObjects( int serverId, ServerObjectsQueue& queue )
{
	int _cleanupPlacedDays = 1;
	int max_x = 0;
	int max_y = 15360;
	//_logger.warning("Load DB");
	auto worldObjsRes = getDB()->queryParams("select iv.id as id, v.class_name, 0 as owner_id, iv.worldspace, iv.inventory, iv.parts, iv.fuel, iv.damage, 0 as combination from `%s` iv join `world_vehicle` wv on iv.`world_vehicle_id` = wv.`id` join `vehicle` v on wv.`vehicle_id` = v.`id` where iv.`instance_id` = %d union select id.`unique_id`, d.`class_name`, id.`owner_id`, id.`worldspace`, id.`inventory`, `Hitpoints`, `Fuel`, `Damage`, id.combination from `%s` id inner join `deployable` d on id.`deployable_id` = d.`id` where id.`instance_id` = %d AND `deployable_id` IS NOT NULL", _vehTableName.c_str(), serverId, _depTableName.c_str(), serverId);

	//_logger.warning("Loaded DB");
	if (!worldObjsRes)
	{
		_logger.error("Failed to fetch objects from database");
		return;
	}
	//_logger.warning("worldObjsRes->fetchRow");
	while (worldObjsRes->fetchRow())
	{
		//_logger.warning("Start Code");
		auto row = worldObjsRes->fields();
		Sqf::Parameters objParams;
		objParams.push_back(string("OBJ"));
		//_logger.warning("row 0");
		string objectId = row[0].getString();
		//objParams.push_back(lexical_cast<string>(objectId)); //objectId should be stringified
		objParams.push_back(objectId);
		//_logger.warning("Loaded objParams");
		try
		{
			objParams.push_back(row[1].getString()); //classname
			//_logger.warning("Loaded objectID from row 1 " );
			objParams.push_back(lexical_cast<string>(row[2].getInt32())); //ownerId should be stringified
			//_logger.warning("Loaded objectID from row 2 ");
			Sqf::Value worldSpace = lexical_cast<Sqf::Value>(row[3].getString()); //worldspace
			//_logger.warning("Loaded objectID from row 3 " );
			if (_objectOOBReset)
			{
				//_logger.warning("oobreset");
				PositionInfo posInfo = FixOOBWorldspace(worldSpace, max_x, max_y);
				if (posInfo.is_initialized())
					_logger.warning("Reset ObjectID " + lexical_cast<string>(objectId) + " (" + row[1].getString() + ") from position " + lexical_cast<string>(*posInfo));

			}			
			//_logger.warning("pushback worldspace");
			objParams.push_back(worldSpace);

			//Inventory can be NULL
			{
				
				string invStr = "[]";
				if (!row[4].isNull())
					invStr = row[4].getString(); //inventory
				//_logger.warning("Loaded objectID from row 4 "); 
				objParams.push_back(lexical_cast<Sqf::Value>(invStr));
			}	
			
			objParams.push_back(lexical_cast<Sqf::Value>(row[5].getCStr())); //Damage
			//_logger.warning("Loaded objectID from row 5 ");
			objParams.push_back(row[6].getDouble()); //Hitpoints
			//_logger.warning("Loaded objectID from row 6 "); 
			objParams.push_back(row[7].getDouble()); //Fuel
			//_logger.warning("Loaded objectID from row 7 " );
			string objectId = row[8].getString();
			//stmt->addInt64(uniqueId); //unique_id
			//stmt->addString(characterId); //owner_id
			//stmt->addInt32(serverId); //instance_id
			//stmt->addString(lexical_cast<string>(worldSpace)); //worldspace
			//stmt->addString(lexical_cast<string>(inventory)); //inventory
			//stmt->addDouble(damage); //Damage
			//stmt->addString(lexical_cast<string>(hitPoints)); //Hitpoints
			//stmt->addDouble(fuel); //Fuel
		}
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping ObjectID " + lexical_cast<string>(objectId) + " load because of invalid data in db");
			continue;
		}

		queue.push(objParams);
	}
}

bool SqlObjDataSource::updateObjectInventory( int serverId, Int64 objectIdent, bool byUID, const Sqf::Value& inventory )
{
	unique_ptr<SqlStatement> stmt;

	if (byUID) // infer that if byUID, it is a deployable - by id, a vehicle
	{
		stmt = getDB()->makeStatement(_stmtUpdateObjectByUID, "update `"+_depTableName+"` set `inventory` = ? where `unique_id` = ? and `instance_id` = ?");
	}
	else
	{
		stmt = getDB()->makeStatement(_stmtUpdateObjectByID, "update `"+_vehTableName+"` set `inventory` = ? where `id` = ? and `instance_id` = ?");
	}

	stmt->addString(lexical_cast<string>(inventory));
	stmt->addInt64(objectIdent);
	stmt->addInt32(serverId);

	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlObjDataSource::deleteObject( int serverId, Int64 objectIdent, bool byUID )
{
	unique_ptr<SqlStatement> stmt;
	if (byUID) // infer that if byUID, it is a deployable - by id, a vehicle
	{
		stmt = getDB()->makeStatement(_stmtDeleteObjectByUID, "delete from `"+_depTableName+"` where `unique_id` = ? and `instance_id` = ?");
	}
	else
	{
		stmt = getDB()->makeStatement(_stmtDeleteObjectByID, "delete from `"+_vehTableName+"` where `id` = ? and `instance_id` = ?");
	}
	stmt->addInt64(objectIdent);
	stmt->addInt32(serverId);

	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlObjDataSource::updateVehicleMovement( int serverId, Int64 objectIdent, const Sqf::Value& worldSpace, double fuel )
{
	auto stmt = getDB()->makeStatement(_stmtUpdateVehicleMovement, "update `"+_vehTableName+"` set `worldspace` = ? , `fuel` = ? where `id` = ? and `instance_id` = ?");
	stmt->addString(lexical_cast<string>(worldSpace));
	stmt->addDouble(fuel);
	stmt->addInt64(objectIdent);
	stmt->addInt32(serverId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlObjDataSource::updateVehicleStatus( int serverId, Int64 objectIdent, const Sqf::Value& hitPoints, double damage )
{
	auto stmt = getDB()->makeStatement(_stmtUpdateVehicleStatus, "update `"+_vehTableName+"` set `parts` = ?, `damage` = ? where `id` = ? and `instance_id` = ?");
	stmt->addString(lexical_cast<string>(hitPoints));
	stmt->addDouble(damage);
	stmt->addInt64(objectIdent);
	stmt->addInt32(serverId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

//bool SqlObjDataSource::createObject( int serverId, const string& className, string characterId, const Sqf::Value& worldSpace, Int64 objectIdent )
bool SqlObjDataSource::createObject( int serverId, const string& className, double damage, int characterId, 
	const Sqf::Value& worldSpace, const Sqf::Value& inventory, const Sqf::Value& hitPoints, double fuel, Int64 uniqueId, int combinationId )
{
	//1:TentStorage:0:3:[329,[11173,3155.13,0.00391388]]:[]:[]:0:111730315510329:|
	auto stmt = getDB()->makeStatement(_stmtCreateObject, 
		"insert into `"+_depTableName+"` (`unique_id`, `deployable_id`, `owner_id`, `instance_id`, `worldspace`, `inventory`, `Damage`,`Hitpoints`, `Fuel`, `created`, combination ) "
		"select ?, d.id, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, ? from deployable d where d.class_name = ?");
		//"insert into `"+_depTableName+"` (`unique_id`, `deployable_id`, `owner_id`, `instance_id`, `worldspace`, `created`) "
		//"select ?, d.id, ?, ?, ?, CURRENT_TIMESTAMP from deployable d where d.class_name = ?");
	//"select uniqueId, d.id, characterId, serverId, worldSpace, inventory, damage, hitPoints, fuel, CURRENT_TIMESTAMP from deployable d where d.class_name = className");
	stmt->addInt64(uniqueId); //unique_id
	stmt->addInt32(characterId); //owner_id
	stmt->addInt32(serverId); //instance_id
	stmt->addString(lexical_cast<string>(worldSpace)); //worldspace
	stmt->addString(lexical_cast<string>(inventory)); //inventory
	stmt->addDouble(damage); //Damage
	stmt->addString(lexical_cast<string>(hitPoints)); //Hitpoints
	stmt->addDouble(fuel); //Fuel
	stmt->addInt32(combinationId); //combination
	stmt->addString(lexical_cast<string>(className));
	//stmt->addInt64(objectIdent);
	//stmt->addString(characterId);
	//stmt->addInt32(serverId);
	//stmt->addString(lexical_cast<string>(worldSpace));
	//stmt->addString(lexical_cast<string>(className));
	bool exRes = stmt->execute();
	poco_assert(exRes == true);
	//_logger.error("Statement " + lexical_cast<string>(_stmtCreateObject) );

	return exRes;
}

