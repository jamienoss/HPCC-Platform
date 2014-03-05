/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */
%option 8bit never-interactive
%option prefix="ecl2yy"
%option nounistd
%option reentrant
%option noyywrap
%option case-insensitive
%option bison-bridge
%option yylineno
%option stack
  //%option nodefault


%{
#include "platform.h"
#include "eclparser.hpp"
#include "eclgram.h"
#include "hqlerrors.hpp"
#include <iostream>
#include <cstring>

class ParserData;

#define YYSTYPE ParserData

#define YY_DECL extern int ecl2yylex(YYSTYPE * yylval_param, EclParser * parser, yyscan_t yyscanner)

  //--------------------------------------------------------------------------------
static unsigned hex2digit(char c)
{
  if (c >= 'a')
    return (c - 'a' + 10);
  else if (c >= 'A')
    return (c - 'A' + 10);
  return (c - '0');
}
  //--------------------------------------------------------------------------------
static __int64 str2int64(unsigned len, const char * digits, unsigned base)
{
  __int64 value = 0;
  while (len--)
  {
    char c = *digits++;
    value = value * base + hex2digit(c);
  }
  return value;
}
  //--------------------------------------------------------------------------------
static void hex2str(char * target, const char * digits, unsigned len)
{
  while (len)
  {
    *target++ = (hex2digit(digits[0]) << 4) | hex2digit(digits[1]);
    len -= 2;
    digits += 2;
  }
}
  //--------------------------------------------------------------------------------
static inline void setInteger(YYSTYPE * yylval, char * tokenText, unsigned txtLen, int lineno, EclLexer & lexer, __int64 _value, unsigned size, bool isSigned)
{
    //std::cout << tokenText << "\n";

    yylval->kind = INTEGER_CONST;
    yylval->value = createIntValue(_value, size, isSigned);

    yylval->setEclLocations(lineno, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
    lexer.updatePos(txtLen);
}
  //--------------------------------------------------------------------------------
static inline void setReal(YYSTYPE * yylval, char * tokenText, unsigned txtLen, int lineno, EclLexer & lexer)
{
    //std::cout << tokenText << "\n";

    yylval->kind = FLOAT_CONST;
    yylval->value = createRealValue(atof(tokenText), 8);

    yylval->setEclLocations(lineno, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
    lexer.updatePos(txtLen);
}
  //--------------------------------------------------------------------------------
static inline void setDecimal(YYSTYPE * yylval, char * tokenText, unsigned txtLen, int lineno, EclLexer & lexer, EclParser * parser)
{
    //std::cout << tokenText << "\n";

    yylval->kind = DECIMAL_CONST;
    const char * dot = strchr(tokenText, '.');
    Owned<ITypeInfo> type;
    if(dot)
    {
        unsigned before = (size32_t)(dot-tokenText);
        unsigned after = txtLen-2 - before;
        if (before > MAX_DECIMAL_LEADING)
        {
            StringBuffer msg = "Decimal constant contains too many integral digits: Max allowed = ";
            msg.append(MAX_DECIMAL_LEADING);
            parser->reportError(ERR_ILLSIZE_DECIMAL, msg, lexer.sourcePath->str(), lineno, lexer.yyColumn, lexer.yyPosition);
            //lexer->reportError(returnToken, ERR_ILLSIZE_DECIMAL, "Decimal constant contains too many integral digits (>%d)", MAX_DECIMAL_LEADING);
            before = MAX_DECIMAL_LEADING;
            after = 0;
        }
        if (after > MAX_DECIMAL_PRECISION)
        {
            StringBuffer msg = "Decimal constant may lose significant digits; Max allowed = ";
            msg.append(MAX_DECIMAL_PRECISION);
            parser->reportError(ERR_ILLSIZE_DECIMAL, msg, lexer.sourcePath->str(), lineno, lexer.yyColumn, lexer.yyPosition);
            //lexer->reportWarning(returnToken, ERR_ILLSIZE_DECIMAL, "Decimal constant may lose significant digits (>%d)", MAX_DECIMAL_PRECISION);
            after = MAX_DECIMAL_PRECISION;
        }
        type.set(makeDecimalType(before+after, after, true));
    }
    else
    {
        unsigned digits = txtLen-1;
        if (digits > MAX_DECIMAL_LEADING)
        {
            //lexer.reportError(returnToken, ERR_ILLSIZE_DECIMAL, "Decimal constant contains too many digits (>%d)", MAX_DECIMAL_LEADING);
            StringBuffer msg = "Decimal constant contains too many digits";
            parser->reportError(ERR_ILLSIZE_DECIMAL, msg, lexer.sourcePath->str(), lineno, lexer.yyColumn, lexer.yyPosition);
            digits = MAX_DECIMAL_LEADING;
        }
        type.set(makeDecimalType(digits, 0, true));
    }
    yylval->value = type->castFrom(txtLen-1, tokenText);

    yylval->setEclLocations(lineno, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
    lexer.updatePos(txtLen);
}
  //--------------------------------------------------------------------------------
static inline void setString(YYSTYPE * yylval, char * tokenText, unsigned txtLen, int lineno, EclLexer & lexer)
{
    //std::cout << tokenText << "\n";

    yylval->kind = STRING_CONST;
    yylval->value = createStringValue(tokenText, 32); //Might need to be UTF8

    yylval->setEclLocations(lineno, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
    lexer.updatePos(txtLen);
}
  //--------------------------------------------------------------------------------
static inline void setBool(YYSTYPE * yylval, char * tokenText, unsigned txtLen, int lineno, EclLexer & lexer)
{
    //std::cout << tokenText << "\n";

    yylval->kind = BOOLEAN_CONST;
    yylval->value = createBoolValue(tokenText);

    yylval->setEclLocations(lineno, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
    lexer.updatePos(txtLen);
}
  //--------------------------------------------------------------------------------
static inline void setLex(YYSTYPE * yylval, unsigned tokenKind, unsigned txtLen, int lineno, EclLexer & lexer)
{
    //std::cout << (char)tokenKind << "\n";

    yylval->kind = tokenKind;
    yylval->setEclLocations(lineno, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
    lexer.updatePos(txtLen);
}
  //--------------------------------------------------------------------------------
static inline void setId(YYSTYPE * yylval, char * tokenText, unsigned txtLen, int lineno, EclLexer & lexer)
{
    //std::cout << tokenText << "\n";

    yylval->kind = ID;
    yylval->setEclLocations(lineno, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
    yylval->id = createIdAtom(tokenText, txtLen);
    lexer.updatePos(txtLen);
}
  //--------------------------------------------------------------------------------
static char foldMinus(char * & tokenText, unsigned txtLen)
{
    if(txtLen == 1)
        return '-';

    unsigned count = 1;
    while(*(tokenText)++)
    {
        if(*tokenText == '-')
            ++count;
    }

    if(count%2)
      return '+';

    return '-';
}

#define manSetInt(value, size, isSigned) setInteger(yylval, yytext, yyleng, yylineno, parser->getLexer(), value, size, isSigned)
#define setInt setInteger(yylval, yytext, yyleng, yylineno, parser->getLexer(), str2int64(yyleng, yytext, 10), 8, false)
#define setReal setReal(yylval, yytext, yyleng, yylineno, parser->getLexer())
#define setDecimal setDecimal(yylval, yytext, yyleng, yylineno, parser->getLexer(), parser)
#define setString setString(yylval, yytext, yyleng, yylineno, parser->getLexer())
#define setBool setBool(yylval, yytext, yyleng, yylineno, parser->getLexer())
#define setLex(x) setLex(yylval, x, yyleng, yylineno, parser->getLexer())
#define setId setId(yylval, yytext, yyleng, yylineno, parser->getLexer())
#define updatepos parser->getLexer().updatePos(yyleng)
#define resetpos parser->getLexer().resetPos()

#define yyterminate() setLex(_EOF_);return _EOF_

%}

letter        [_a-zA-Z]
digit         [0-9]
bindigit      [0-1]
hexdigit      [a-fA-F0-9]
alphanum      [_$a-zA-Z0-9]
alphanumcolon [a-z_A-Z$_0-9:@] //extra underscore not needed
blank         [ \t\r]
slash         [/]
star          [*]
percent       [%]
lcurly        [\{]
rcurly        [\}]
dot           [\.]
hexpairs      ({hexdigit}{hexdigit})+
err_hexpairs  {hexdigit}({hexdigit}{hexdigit})*
xpathchars    [a-z_A-Z$_0-9:/\[\]@=!]
xpathseq      ([^}\r\n])+

%x COMMENT
%x SLCOMMENT

%%

"/*"                     { updatepos; BEGIN(COMMENT); }
<COMMENT>"*/"            { updatepos; BEGIN(0); }
<COMMENT>[^*\n]+         { updatepos; }
<COMMENT>\n              { resetpos; }
<COMMENT>"*"             { updatepos; }

\n                       { resetpos; }
{blank}+                 { updatepos; }

"//"                     { updatepos; BEGIN(SLCOMMENT); }
<SLCOMMENT>\n            { updatepos; BEGIN(0); }
<SLCOMMENT>[^\n]+        { updatepos; }

AS                       { setLex(AS); return AS; }
END                      { setLex(END); return END; }
FALSE                    { setBool; return BOOLEAN_CONST; }
FROM                     { setLex(FROM); return FROM; }
IFBLOCK                  { setLex(IFBLOCK); return IFBLOCK; }
IMPORT                   { setLex(IMPORT); return IMPORT; }
RECORD                   { setLex(RECORD); return RECORD; }
TRUE                     { setBool; return BOOLEAN_CONST; }
TYPE                     { setLex; return TYPE; }
".^"                     { setLex(UPDIR); return UPDIR; }
".."                     { setLex; return DOTDOT; }



{digit}+                 { manSetInt(str2int64(yyleng, yytext, 10), 8, true); return INTEGER_CONST; }
{digit}+U                { manSetInt(str2int64(yyleng-1, yytext, 10), 8, false); return INTEGER_CONST; }

0x{hexdigit}+            { manSetInt(str2int64(yyleng-2, yytext+2, 16), 8, true); return INTEGER_CONST;  }
0x{hexdigit}+U           { manSetInt(str2int64(yyleng-3, yytext+2, 16), 8, false); return INTEGER_CONST;  }
{digit}{hexdigit}+X      { manSetInt(str2int64(yyleng-1, yytext, 16), 8, true); return INTEGER_CONST;  }
{digit}{hexdigit}+XU     { manSetInt(str2int64(yyleng-2, yytext, 16), 8, false); return INTEGER_CONST;  }

0b{bindigit}+            { manSetInt(str2int64(yyleng-2, yytext+2, 2), 8, true); return INTEGER_CONST; }
0b{bindigit}+U           { manSetInt(str2int64(yyleng-3, yytext+2, 2), 8, false); return INTEGER_CONST; }
{bindigit}+B             { manSetInt(str2int64(yyleng-1, yytext, 2), 8, true); return INTEGER_CONST; }
{bindigit}+BU            { manSetInt(str2int64(yyleng-2, yytext, 2), 8, false); return INTEGER_CONST; }

{digit}*\.{digit}+((e|E)("+"|"-")?{digit}+)? {
                          setReal; return FLOAT_CONST; }
{digit}*\.?{digit}+(d|D) { setDecimal; return DECIMAL_CONST; }

\'[^\n]*\'               { setString; return STRING_CONST; }

{letter}{alphanum}*      { setId; return ID; }
${digit}+                { setId; return PARSE_ID; }

":="                     { setLex(ASSIGN); return ASSIGN; }
";"+[; \t\r]*            { setLex(';'); return ';'; }
"!="                     { setLex(NE); return NE; }
"="                      { setLex(EQ); return EQ; }
"<="                     { setLex(LE); return LE; }
">="                     { setLex(GE); return GE; }//x.<y>=n should be interpreted as x.<y> = n rather than x.<y >= n (NEEDS FIXING)
"<"                      { setLex(LT); return LT; }
">"                      { setLex(GT); return GT; }

"+"+[+ \t\r]*            { setLex('+'); return '+'; } /*might as well handle this here, though doesn't fold over new lines*/
"-"+[- \t\r]*            { char op = foldMinus(yytext, yyleng); setLex(op); return op; }

.                        { updatepos; setLex(yytext[0]); return (yytext[0]); }

%%

int yyerror(const char *msg) {
    fprintf(stderr,"Error:%s\n",msg); return 0;
}