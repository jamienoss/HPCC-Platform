#include "newparser.hpp"
#include "eclparser.hpp"


bool doNewParseQuery(IFileContents * contents)
{
    Owned<EclParser> parser;
    parser.setown(new EclParser(contents));

    return !(parser->parse());
}
