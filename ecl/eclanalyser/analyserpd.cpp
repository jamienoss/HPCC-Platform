#include "analyserpd.hpp"
#include "asyntaxtree.hpp"
#include "bisongram.h"


ParserData & AnalyserPD::add(const TokenKind & _kind, const ECLlocation & _pos)
{
    if(!node)
        node.set(createSyntaxTree(_kind, _pos));
    else
        node->addChild(createSyntaxTree(_kind, _pos));

    return *this;
}

ParserData & AnalyserPD::add(const AnalyserPD & token2add)
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


ISyntaxTree * AnalyserPD::createSyntaxTree(const TokenKind & _kind, const ECLlocation & _pos)
{
    return createAnalyserPuncST(_pos, _kind);
}

ISyntaxTree * AnalyserPD::createSyntaxTree(const AnalyserPD & token2add)
{
    switch(token2add.kind)
    {
    //case BOOLEAN :
    //case INTEGER :
    //case DECIMAL :
    //case FLOAT : return createConstSyntaxTree(token2add.pos, value); break;
    case NONTERMINAL :
    case TERMINAL : return createAnalyserIdST(token2add.pos, token2add.id, token2add.kind); break;
    default : return createAnalyserPuncST(token2add.pos, token2add.kind);
    }
}
