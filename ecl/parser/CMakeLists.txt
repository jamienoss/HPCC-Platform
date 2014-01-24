################################################################################
#    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
################################################################################


# Component: parser

#####################################################
# Description:
# ------------
#    Cmake Input File for parser
#####################################################


project( parser )

set (   SRCS
        eclgram.y
        ecllex.l

        parserdata.hpp
        parserdata.cpp
        syntaxtree.hpp
        syntaxtree.cpp
        eclparser.hpp
        eclparser.cpp
    )

include_directories (
         ${CMAKE_CURRENT_BINARY_DIR}  # for generated header files
         ${CMAKE_BINARY_DIR}
         ${CMAKE_BINARY_DIR}/oss
         ./../../common/deftype
         ./../../system/hrpc
         ./../../common/thorhelper
         ./../../rtl/eclrtl
         ./../eclagent
         ./../../system/include
         ./../../common/workunit
         ./../../system/jlib
         ./../../ecl/hql
         ./../../ecl/parser
         ./../../testing/unittests
    )

if (WIN32)
    add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/eclgram.cpp ${CMAKE_CURRENT_BINARY_DIR}/eclgram.h
        #pipe result through findstr to remove warnings that are hard to suppress, and pipe through more to prevent error code from no matches aborting the compile
        COMMAND ${bisoncmdprefix} ${bisoncmd} --report=state --defines=${CMAKE_CURRENT_BINARY_DIR}/eclgram.h --output=${CMAKE_CURRENT_BINARY_DIR}/eclgram.cpp ${CMAKE_CURRENT_SOURCE_DIR}/eclgram.y 2>&1 | findstr /V "unused value" | more
        DEPENDS eclgram.y
    )
else()
    add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/eclgram.cpp ${CMAKE_CURRENT_BINARY_DIR}/eclgram.h
        #pipe result through grep to remove warnings that are hard to suppress, and pipe through cat to prevent error code from no matches aborting the compile
        COMMAND ${bisoncmdprefix} ${bisoncmd} --report=state --defines=${CMAKE_CURRENT_BINARY_DIR}/eclgram.h --output=${CMAKE_CURRENT_BINARY_DIR}/eclgram.cpp ${CMAKE_CURRENT_SOURCE_DIR}/eclgram.y 2>&1 | grep -v "unused value" | cat
        DEPENDS eclgram.y
    )
endif()

add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ecllex.cpp
    COMMAND ${flexcmdprefix} ${flexcmd} --outfile=${CMAKE_CURRENT_BINARY_DIR}/ecllex.cpp --header-file=${CMAKE_CURRENT_BINARY_DIR}/ecllex.hpp ${CMAKE_CURRENT_SOURCE_DIR}/ecllex.l
    DEPENDS ecllex.l
)

if (WIN32)
    set_source_files_properties (eclgram.cpp PROPERTIES COMPILE_FLAGS "/Od")
endif()

#ADD_DEFINITIONS( -D_USRDLL -DHQL_EXPORTS -DHQLFOLD_EXPORTS -DHQLTRANS_EXPORTS )

HPCC_ADD_LIBRARY( parser SHARED ${SRCS} ${CMAKE_CURRENT_BINARY_DIR}/eclgram.cpp ${CMAKE_CURRENT_BINARY_DIR}/ecllex.cpp  )

install ( TARGETS parser RUNTIME DESTINATION ${EXEC_DIR} LIBRARY DESTINATION ${LIB_DIR} )
target_link_libraries ( parser
         jlib
         nbcd
         eclrtl
         deftype
         ${CPPUNIT_LIBRARIES}
    )
