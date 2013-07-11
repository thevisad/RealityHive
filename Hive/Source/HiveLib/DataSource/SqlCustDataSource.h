/*
* Copyright (C) 2009-2012 Andrew DeLisa <http://github.com/ayan4m1/hive>
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

#pragma once

#include "SqlDataSource.h"
#include "CustDataSource.h"

class SqlCustDataSource : public SqlDataSource, public CustDataSource
{
public:
	SqlCustDataSource(Poco::Logger& logger, shared_ptr<Database> db);
	~SqlCustDataSource();

	void populateQuery( string query, Sqf::Parameters& params, CustomDataQueue& queue ) override;
	bool customExecute( string query, Sqf::Parameters& params ) override;
};
