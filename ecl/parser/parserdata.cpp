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
}

ParserData::~ParserData()
{
   //Release(value);
   //Release(id);
    //node.clear();
}

ISyntaxTree * ParserData::getNode()
{
    return node.get();
}

const ECLlocation & ParserData::queryNodePosition() const
{
    return node->queryPosition();
}

ParserData & ParserData::first()
{
    pos.clear();
    kind = 0;
    node.set(NULL);//needs to change
    return *this;
}

ParserData & ParserData::first(const ParserData & token2add)
{
    first();
    return add(token2add);
}

ParserData & ParserData::first(const TokenKind & _kind, const ECLlocation & _pos)
{
    first();
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
    case BOOLEAN :
    case CHARACTER :
    case DECIMAL :
    case FLOAT:
    case INTEGER  : return createConstSyntaxTree(token2add.pos, value); break;
    case ID : return createIdSyntaxTree(token2add.pos, token2add.id); break;
    default : return createPuncSyntaxTree(token2add.pos, token2add.kind);
    }
}

ISyntaxTree * ParserData::createSyntaxTree(const TokenKind & _kind, const ECLlocation & _pos)
{
    return createPuncSyntaxTree(_pos, _kind);
}
