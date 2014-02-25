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
    return (node != NULL);
}

ISyntaxTree * ParserData::queryNode() const
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
    bool isNode = token2add.isNode();
	if(!isNode)
	    node.set(createSyntaxTree(token2add));
	else// this propergates possible emptyNode up tree, do we want this? Perhaps.
	    node.set(token2add.node);

	return *this;
}

ParserData & ParserData::setNode(const TokenKind & _kind, const ECLlocation & _pos)
{
    clearToken();
	nodeKind = parent;
    node.set(createSyntaxTree(_kind, _pos));
    return *this;
}

ParserData & ParserData::addChild(const TokenKind & _kind, const ECLlocation & _pos)
{
    node->addChild(createSyntaxTree(_kind, _pos));
    return *this;
}

ParserData & ParserData::addChild(const ParserData & token2add)
{
    bool isNode = token2add.isNode();
	bool emptyToken2add  = (token2add.nodeKind == emptyParent);

	if(isNode)
	{
	    if(!emptyToken2add)
	        node->addChild(token2add.node);
	    else
	        node->transferChildren(token2add.node);
	}
	else
        node->addChild(createSyntaxTree(token2add));

    return *this;
}

ISyntaxTree * ParserData::createSyntaxTree(const ParserData & token2add)
{
    switch(token2add.kind)
    {
    case BOOLEAN_CONST :
    case STRING_CONST :
    case DECIMAL_CONST :
    case FLOAT_CONST:
    case INTEGER_CONST  : return createConstSyntaxTree(token2add.pos, value); break;
    case ID : return createIdSyntaxTree(token2add.pos, token2add.id); break;
    default : return createPuncSyntaxTree(token2add.pos, token2add.kind);
    }
}

ISyntaxTree * ParserData::createSyntaxTree(const TokenKind & _kind, const ECLlocation & _pos)
{
    return createPuncSyntaxTree(_pos, _kind);
}
