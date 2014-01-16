/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.


    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#ifndef TOKENDATA_HPP
#define TOKENDATA_HPP

#include "hql.hpp"
//#include "eclgram.h"

//extern YYTOKENTYPE;

enum symbolKind
{
    none,
	integerKind,
	unsignedKind,
	realKind,
    idKind,
	lexemeKind
};
//----------------------------------TokenData--------------------------------------------------------------------
class TokenData
{
public:
    ECLlocation * pos;
	symbolKind attributeKind;
	//yytokentype attributeKind;
	union
	{
        int integer;
        float real;
        IIdAtom * name;
	};

	void setEclLocations(int lineNo, int column, int position, ISourcePath * sourcePath);
};

class StringBuffer;
void appendParserTokenText(StringBuffer & target, unsigned tok);

#endif
