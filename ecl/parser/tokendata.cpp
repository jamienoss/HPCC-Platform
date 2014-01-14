#include "tokendata.hpp"
#include <cstring>
#include <iostream>

void TokenData::cpy(TokenData &tok) // Discuss with Gavin. At present this is implemented to allow for char * lexeme
{
	switch (tok.attributeKind) {
	case (lexemeKind) : {
		unsigned txtLen = strlen(tok.lexeme) + 1;
		lexeme = new char [txtLen];
		memcpy(lexeme, tok.lexeme, txtLen);
		break;
	}
	case (integerKind) : integer = tok.integer; break;
	case (realKind) : real = tok.real; break;
	}

	attributeKind = tok.attributeKind;

    pos = new ECLlocation();
    pos->set(*tok.pos);
}

void TokenData::setEclLocations(int lineNo, int column, int position, ISourcePath * sourcePath)
{
    if(!pos)
        pos = new ECLlocation();

    pos->set(lineNo, column, position, sourcePath);
}
