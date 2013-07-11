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

#include "CustDataSource.h"

#include <boost/algorithm/string/predicate.hpp>

/*string CustDataSource::BuildSQL( string query, int argCount )
{
	string ret = "call `"+procName+"`(";	
	for( int i=0;i<argCount;i++ )
	{
		if (i != 0 && ((i + 1) != argCount)) {
			ret += ", ";
		}
		ret += "?";
	}
	ret += ")";
	return ret;
}*/