#include "tokendata.hpp"
#include <cstring>
#include <iostream>

void TokenData::setEclLocations(int lineNo, int column, int position, ISourcePath * sourcePath)
{
    if(!pos)
        pos = new ECLlocation();

    pos->set(lineNo, column, position, sourcePath);
}
