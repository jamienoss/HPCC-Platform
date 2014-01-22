#include "tokendata.hpp"
#include <cstring>
#include <iostream>
#include"syntaxtree.hpp"

void TokenData::setEclLocations(int lineNo, int column, int position, ISourcePath * sourcePath)
{
    pos.set(lineNo, column, position, sourcePath);
}

TokenData::TokenData()
{
    pos.clear();
    tokenKind = 0;
    integer = 0;
}

TokenData::~TokenData() {}

TokenData & TokenData::clear()
{
    pos.clear();
    tokenKind = 0;
    integer = 0;
    node.set(NULL);
    return *this;
}

TokenData & TokenData::clear(TokenData & token)
{
    clear();
    return add(token);
}

TokenData & TokenData::clear(TokenKind token, const ECLlocation & _pos)
{
    clear();
    return add(token, _pos);
}

TokenData & TokenData::add(TokenKind token, const ECLlocation & _pos)
{
    if(!node)
        node.set(createSyntaxTree(token, _pos));
    else
        node->addChild(createSyntaxTree(token, _pos));

    return *this;
}

TokenData & TokenData::add(TokenData & token)
{
    if(!token.node && !node)
    {
        node.set(createSyntaxTree(token));
    }
    else if(token.node && node)
    {
        node->addChild(token.node);
    }
    else if(token.node && !node)
    {
        //node.set(SyntaxTree::createSyntaxTree(token));//not sure about this (token) vs ()???
        //node->addChild(token.node);
        node.set(token.node);
    }
    else if(!token.node && node)
    {
        node->addChild(createSyntaxTree(token));
    }
    return *this;
}
