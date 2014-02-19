#include "analyserpd.hpp"
#include "asyntaxtree.hpp"
#include "bisongram.h"
#include <iostream>


ParserData & AnalyserPD::addChild(const TokenKind & _kind, const ECLlocation & _pos)
{
    if(!node)
        node.set(createSyntaxTree(_kind, _pos));
    else
        node->addChild(createSyntaxTree(_kind, _pos));

    return *this;
}

ParserData & AnalyserPD::addChild(const ParserData & token2add)
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
            node->addChild(token2add.getNode());
        else
            node->transferChildren(token2add.getNode());
    }
    else if(isNode && !node)
    {
        node.set(token2add.getNode());
    }
    else if(!isNode && node)
    {
        node->addChild(createSyntaxTree(token2add));
    }
    return *this;
}

ParserData & AnalyserPD::setEmptyNode()
{
    //clearToken();
    node.clear();
    nodeKind = emptyParent;
    node.set(createSyntaxTree(0, pos));//Might want to better this?
    return *this;
}

ISyntaxTree * AnalyserPD::createSyntaxTree(const TokenKind & _kind, const ECLlocation & _pos)
{
    return createAnalyserPuncST(_pos, _kind);
}

ISyntaxTree * AnalyserPD::createSyntaxTree(const ParserData & token2add)
{
    switch(token2add.kind)
    {
    //case BOOLEAN :
    //case INTEGER :
    //case DECIMAL :
    //case FLOAT : return createConstSyntaxTree(token2add.pos, value); break;
    case 300 :
    case NONTERMINAL :
    case TERMINAL : return createAnalyserIdST(token2add.pos, token2add.id, token2add.kind); break;
    case CODE : return createAnalyserStringST(token2add.pos, *token2add.text, token2add.kind); break;
    default : return createAnalyserPuncST(token2add.pos, token2add.kind);
    }
}
