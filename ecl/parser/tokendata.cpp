#include "tokendata.hpp"
#include <cstring>
#include <iostream>
#include"syntaxtree.hpp"

void TokenData::setEclLocations(int lineNo, int column, int position, ISourcePath * sourcePath)
{
    pos.set(lineNo, column, position, sourcePath);
}

void TokenData::createSyntaxTree()
{
    node.set(SyntaxTree::createSyntaxTree());
}

void TokenData::createSyntaxTree(TokenData & token)
{
    node.set(SyntaxTree::createSyntaxTree(token));
}


void TokenData::createSyntaxTree(TokenKind token, const ECLlocation & _pos)
{
    node.set(SyntaxTree::createSyntaxTree(token, _pos));
}

void TokenData::createSyntaxTree(TokenData & parentTok, TokenData & leftTok, TokenData & rightTok)
{
    node.set(SyntaxTree::createSyntaxTree(parentTok, leftTok, rightTok));
}

void TokenData::createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch, TokenData & rightTok)
{
    node.set(SyntaxTree::createSyntaxTree(parentTok, leftBranch, rightTok));
}

void TokenData::createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch)
{
    node.set(SyntaxTree::createSyntaxTree(parentTok, leftBranch));
}

void TokenData::createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch, ISyntaxTree * rightBranch)
{
    node.set(SyntaxTree::createSyntaxTree(parentTok, leftBranch, rightBranch));
}

TokenData::TokenData() {}

TokenData::~TokenData() {}
