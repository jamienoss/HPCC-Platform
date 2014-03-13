#include "semantics.hpp"

IHqlExpression * semantics(ISyntaxTree * node)
{
    unsigned nChildren = node->numberOfChildren();
    if(!nChildren)
    {
        return node->translate();
    }
    else if(nChildren == 2)
    {
        return node->translate(semantics(node->getChild(0)), semantics(node->getChild(1)));
    }

    HqlExprArray children;
    ForEachItemIn(i, node->queryChildren())
    {
        children.append(*semantics(node->getChild(i)));
    }
    return node->translate(children);
}

IHqlExpression * SyntaxTree::translate()
{
    return createConstantOne();
}

IHqlExpression *  SyntaxTree::translate(IHqlExpression * e1, IHqlExpression * e2)
{
    return createConstantOne();
}

IHqlExpression *  SyntaxTree::translate(HqlExprArray & hqlChildren)
{
    return createConstantOne();
}







IHqlExpression * ConstantSyntaxTree::translate()
{
    return createConstant(value);
}


IHqlExpression *  PuncSyntaxTree::translate(IHqlExpression * e1, IHqlExpression * e2)
{
    return createValue(no_add, e1, e2);
}
