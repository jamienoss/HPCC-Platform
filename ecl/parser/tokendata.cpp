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
    node.setown(*LINK(SyntaxTree::createSyntaxTree()));
}

void TokenData::createSyntaxTree(TokenData & token)
{
    node = SyntaxTree::createSyntaxTree(token);
}


void TokenData::createSyntaxTree(TokenKind token, const ECLlocation & _pos)
{
    node = SyntaxTree::createSyntaxTree(token, _pos);

}

void TokenData::createSyntaxTree(TokenData & parentTok, TokenData & leftTok, TokenData & rightTok)
{
    node = SyntaxTree::createSyntaxTree(parentTok, leftTok, rightTok);
}

void TokenData::createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch, TokenData & rightTok)
{
    node = SyntaxTree::createSyntaxTree(parentTok, leftBranch, rightTok);
}

void TokenData::createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch)
{
    node = SyntaxTree::createSyntaxTree(parentTok, leftBranch);
}

void TokenData::createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch, ISyntaxTree * rightBranch)
{
    node = SyntaxTree::createSyntaxTree(parentTok, leftBranch, rightBranch);
}
