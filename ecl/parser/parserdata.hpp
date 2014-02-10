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

#ifndef PARSERDATA_HPP
#define PARSERDATA_HPP

#include "hql.hpp" //for ECLlocations
#include "defvalue.hpp" //IValue

class ISyntaxTree;
typedef unsigned short TokenKind;

//----------------------------------ParserData--------------------------------------------------------------------
class ParserData
{
public:
    ParserData();
    ~ParserData();

	ParserData & first();
    ParserData & first(const ParserData & token2add);
    ParserData & first(const TokenKind & _kind, const ECLlocation & _pos);
    virtual ParserData & add(const ParserData & token2add);
    virtual ParserData & add(const TokenKind & _kind, const ECLlocation & _pos);

    const ECLlocation & queryNodePosition() const;
    ISyntaxTree * getNode();
    virtual ISyntaxTree * createSyntaxTree(const ParserData & token2add);
    virtual ISyntaxTree * createSyntaxTree(const TokenKind & _kind, const ECLlocation & _pos);
    void setEclLocations(int lineNo, int column, int position, ISourcePath * sourcePath);


public:
    ECLlocation pos;
    unsigned kind;
    union
    {
        IValue * value;
        IIdAtom * id;
    };

protected:
    Owned<ISyntaxTree> node;
};

class StringBuffer;
void appendParserTokenText(StringBuffer & target, unsigned tok);

#endif
