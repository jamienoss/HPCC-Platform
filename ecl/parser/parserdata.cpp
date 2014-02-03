#include "parserdata.hpp"
#include <cstring>
#include <iostream>
#include"syntaxtree.hpp"
#include "eclgram.h"

void ParserData::setEclLocations(int lineNo, int column, int position, ISourcePath * sourcePath)
{
    pos.set(lineNo, column, position, sourcePath);
}

ParserData::ParserData()
{
    pos.clear();
    kind = 0;
    integer = 0;
}

ParserData::~ParserData() {}

ParserData & ParserData::clear()
{
    pos.clear();
    kind = 0;
    integer = 0;
    node.set(NULL);//needs to change perhaps
    return *this;
}

ParserData & ParserData::clear(const ParserData & token2add)
{
    clear();
    return add(token2add);
}

ParserData & ParserData::clear(const TokenKind & _kind, const ECLlocation & _pos)
{
    clear();
    return add(_kind, _pos);
}

ParserData & ParserData::add(const TokenKind & _kind, const ECLlocation & _pos)
{
    if(!node)
        node.set(createSyntaxTree(_kind, _pos));
    else
        node->addChild(createSyntaxTree(_kind, _pos));

    return *this;
}

ParserData & ParserData::add(const ParserData & token2add)
{
    if(!token2add.node && !node)
    {
        node.set(createSyntaxTree(token2add));
    }
    else if(token2add.node && node)
    {
        node->addChild(token2add.node);
    }
    else if(token2add.node && !node)
    {
        node.set(token2add.node);
    }
    else if(!token2add.node && node)
    {
        node->addChild(createSyntaxTree(token2add));
    }
    return *this;
}

ISyntaxTree * ParserData::createSyntaxTree(const ParserData & token2add)
{
    switch(token2add.kind)
    {
    //case BOOLEAN : return createConstSyntaxTree(token2add.pos, token2add.bool); break;
    case INTEGER : return createConstSyntaxTree(token2add.pos, token2add.integer); break;
    case REAL : return createConstSyntaxTree(token2add.pos, token2add.real); break;
    case ID : return createIdSyntaxTree(token2add.pos, token2add.name); break;
    default : return createPuncSyntaxTree(token2add.pos, token2add.kind);
    }
}

ISyntaxTree * ParserData::createSyntaxTree(const TokenKind & _kind, const ECLlocation & _pos)
{
    return createPuncSyntaxTree(_pos, _kind);
}
