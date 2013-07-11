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

#include "SqlCustDataSource.h"
#include "Database/Database.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;

SqlCustDataSource::SqlCustDataSource( Poco::Logger& logger, shared_ptr<Database> db ) : SqlDataSource(logger,db)
{
	//todo: anything?
}

SqlCustDataSource::~SqlCustDataSource() {}

void SqlCustDataSource::populateQuery( string query, Sqf::Parameters& params, CustomDataQueue& queue ) {
	for( int i=0;i<params.size();i++ )
	{
		query = boost::algorithm::replace_nth_copy(query, "?", i, Sqf::GetStringAny(params.at(i)));
	}
	auto custRes = getDB()->queryParams(query.c_str());
	while (custRes->fetchRow())
	{
		Sqf::Parameters custParams;
		for( int i=0;i<custRes->numFields();i++ )
		{
			int val = custRes->at(i).getInt32();
			if (val == 0 && custRes->at(i).getString() != "0")
			{
				custParams.push_back(custRes->at(i).getString());
			}
			else
			{
				custParams.push_back(val);
			}
		}

		queue.push(custParams);
	}
}

bool SqlCustDataSource::customExecute( string query, Sqf::Parameters& params ) {
	static SqlStatementID stmtId;
	auto stmt = getDB()->makeStatement(stmtId, query);
	for( int i=0;i<params.size();i++ )
	{
		//todo: improve the type checking here
		try
		{
			stmt->addInt32(lexical_cast<int>(params.at(i)));
		}
		catch (bad_lexical_cast)
		{
			stmt->addString(lexical_cast<string>(params.at(i)));
		}
	}
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}
