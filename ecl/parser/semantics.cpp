#include "semantics.hpp"
#include "eclgram.h"

IHqlExpression * semantics(ISyntaxTree * node)
{
    IHqlExpression * returnExpr;
    HqlExprArray children;

    unsigned nChildren = node->numberOfChildren();
    if(!nChildren)
    {
        returnExpr = node->translate();
    }
    /*else if(nChildren == 2)
    {
        returnExpr =  node->translate(semantics(node->getChild(0)), semantics(node->getChild(1)));
    }*/
    else
    {
        IHqlExpression * node2add = NULL;
        ForEachItemIn(i, node->queryChildren())
        {
            node2add = semantics(node->getChild(i));
            if(node2add)
            {
                children.append(*node2add);
                node2add = NULL;
            }
        }

        unsigned nNodes2add = children.ordinality();
        if(nNodes2add == 2)
        {
            node2add =  node->translate(&children.item(0), &children.item(1));
            if(node2add)
                return node2add;
        }
        else if(nNodes2add == 1)
        {
            node2add =  node->translate(&children.item(0));
            if(node2add)
                return node2add;
            else
                return &children.item(0);
        }
        returnExpr = node->translate(children);
    }

    return NULL;
}
//---------------------------------------------------------------------------------------------------------------------
IHqlExpression * SyntaxTree::translate() { return NULL; }
IHqlExpression *  SyntaxTree::translate(IHqlExpression * expr) { return NULL; }
IHqlExpression *  SyntaxTree::translate(IHqlExpression * e1, IHqlExpression * e2) { return NULL; }
IHqlExpression *  SyntaxTree::translate(HqlExprArray & hqlChildren) { return NULL; }
//---------------------------------------------------------------------------------------------------------------------


IHqlExpression * ConstantSyntaxTree::translate()
{
    return createLocationAnnotation(createConstant(value), pos);
}

IHqlExpression * IdSyntaxTree::translate()
{
    return createLocationAnnotation(createId(id), pos);
}

IHqlExpression * createValue(node_operator op, IHqlExpression * e1, IHqlExpression * e2, ECLlocation & pos)
{
    return createLocationAnnotation(createValue(op, e1, e2), pos);
}

IHqlExpression * createAssign(IHqlExpression * e1, IHqlExpression * e2, ECLlocation & pos)
{
    return createLocationAnnotation(createAssign(e1, e2), pos);
}

IHqlExpression *  PuncSyntaxTree::translate(IHqlExpression * expr)
{
   return NULL;
}

IHqlExpression *  PuncSyntaxTree::translate(IHqlExpression * e1, IHqlExpression * e2)
{
    node_operator op;
    switch(value)
    {
    case '+' :
        return createValue(no_add, e1, e2, pos);
    case '-' :
        return createValue(no_sub, e1, e2, pos);
    case '*' :
        return createValue(no_mul, e1, e2, pos);
    case '/' :
        return createValue(no_div, e1, e2, pos);
    case ASSIGN :
        return createAssign(e1, e2, pos);
    }

    return NULL;
}
