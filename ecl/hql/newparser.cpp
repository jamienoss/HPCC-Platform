#include "newparser.hpp"
#include "eclparser.hpp"


bool doNewParseQuery(IFileContents * contents, IErrorReceiver * errs)
{
    Owned<EclParser> parser;
    parser.setown(new EclParser(contents, errs));

    return !(parser->parse());
}
