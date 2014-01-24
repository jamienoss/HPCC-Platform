#include "analyserpd.hpp"
#include "asyntaxtree.hpp"
#include <cstring>
#include <iostream>

ParserData & AnalyserPD::add(TokenKind token, const ECLlocation & _pos)
{
    if(!node)
        node.set(AnalyserST::createSyntaxTree(token, _pos));
    else
        node->addChild(AnalyserST::createSyntaxTree(token, _pos));

    return *this;
}

ParserData & AnalyserPD::add(ParserData & token)
{
    if(!token.node && !node)
    {
        node.set(AnalyserST::createSyntaxTree(token));
    }
    else if(token.node && node)
    {
        node->addChild(token.node);
    }
    else if(token.node && !node)
    {
        node.set(token.node);
    }
    else if(!token.node && node)
    {
        node->addChild(AnalyserST::createSyntaxTree(token));
    }
    return *this;
}
