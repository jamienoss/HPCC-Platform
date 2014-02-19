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
   //if(text)
   //    delete(text);
   //node.clear();
}

bool ParserData::isNode() const
{
    return node ? true : false;
}

ISyntaxTree * ParserData::getNode() const
{
    return node.get();
}

const ECLlocation & ParserData::queryNodePosition() const
{
    return node->queryPosition();
}

ParserData & ParserData::clearToken()
{
    pos.clear();
    kind = 0;//This is 'end of file', pick another?
    nodeKind = unknown;
    node.clear();
    return *this;
}

ParserData & ParserData::setEmptyNode()
{
	//clearToken();
	node.clear();
	nodeKind = emptyParent;
	node.set(createSyntaxTree(0, pos));//Might want to better this?
	return *this;
}


ParserData & ParserData::setNode(const ParserData & token2add)
{
    clearToken();
	nodeKind = token2add.nodeKind;
    return addChild(token2add);
}

ParserData & ParserData::setNode(const TokenKind & _kind, const ECLlocation & _pos)
{
    clearToken();
	nodeKind = parent;
    return addChild(_kind, _pos);
}

ParserData & ParserData::addChild(const TokenKind & _kind, const ECLlocation & _pos)
{
    if(!node)
        node.set(createSyntaxTree(_kind, _pos));
    else
        node->addChild(createSyntaxTree(_kind, _pos));

    return *this;
}

ParserData & ParserData::addChild(const ParserData & token2add)
{
    bool isNode = token2add.isNode();
	bool emptyToken2add  = token2add.nodeKind == emptyParent ? true : false;

    if(!isNode && !node)
    {
        node.set(createSyntaxTree(token2add));
    }
    else if(isNode && node)
    {
        if(!emptyToken2add)
            node->addChild(token2add.node);
        else
            node->transferChildren(token2add.node);
    }
    else if(isNode && !node)// this propergates possible emptyNode up tree, do we want this? Perhaps.
    {
        node.set(token2add.node);
    }
    else if(!isNode && node)
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
