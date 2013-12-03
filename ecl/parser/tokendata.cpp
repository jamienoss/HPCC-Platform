#include "tokendata.hpp"
#include <cstring>

void TokenData::cpy(TokenData tok) // Discuss with Gavin. At present this is implemented to allow for char * lexeme
{
	switch (tok.attributeKind) {
	case (lexemeKind) : {
		unsigned txtLen = strlen(tok.lexeme) + 1;
		lexeme = new char [txtLen];
		memcpy(lexeme, tok.lexeme, txtLen);
	}
	case (integerKind) : integer = tok.integer;
	case (realKind) : real = tok.real;
	}

	lineNumber = tok.lineNumber;
	attributeKind = tok.attributeKind;
}
