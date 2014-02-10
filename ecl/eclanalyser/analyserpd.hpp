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

#ifndef ANALYSERPD_HPP
#define ANALYSERPD_HPP

#include "parserdata.hpp"

//----------------------------------ParserData--------------------------------------------------------------------
class AnalyserPD : public ParserData
{
public:
    virtual ISyntaxTree * createSyntaxTree(const AnalyserPD & token2add);
    virtual ISyntaxTree * createSyntaxTree(const TokenKind & _kind, const ECLlocation & _pos);
    virtual ParserData & add(const AnalyserPD & token2add);
    virtual ParserData & add(const TokenKind & _kind, const ECLlocation & _pos);
};

#endif
