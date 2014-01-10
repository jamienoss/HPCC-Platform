#include "tokendata.hpp"
#include <cstring>
#include <iostream>

void TokenData::cpy(TokenData tok) // Discuss with Gavin. At present this is implemented to allow for char * lexeme
{
	switch (tok.attributeKind)
	{
	case lexemeKind :
	case terminalKind :
	case nonTerminalKind :
	case nonTerminalDefKind :
	case productionKind  :
	            {
		unsigned txtLen = strlen(tok.lexeme) + 1;
		lexeme = new char [txtLen];
		memcpy(lexeme, tok.lexeme, txtLen);
		break;
	}
	case integerKind : integer = tok.integer; break;
	case realKind : real = tok.real; break;
	default : { }
	}

	lineNumber = tok.lineNumber;
	attributeKind = tok.attributeKind;
}

void TokenData::setKind(symbolKind kind)
{
    attributeKind = kind;
}
