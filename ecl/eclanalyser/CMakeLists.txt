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


# Component: eclanalyser

#####################################################
# Description:
# ------------
#    Cmake Input File for eclanalyser
#####################################################


project( eclanalyser )

set (   SRCS
        bisongram.y
        bisonlex.l

        tokendata.hpp
        tokendata.cpp
        asyntaxtree.hpp
        asyntaxtree.cpp
        analyserparser.hpp
        analyserparser.cpp
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
         ./../../ecl/eclanalyser
         ./../../testing/unittests
    )

if (WIN32)
    add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bisongram.cpp ${CMAKE_CURRENT_BINARY_DIR}/bisongram.h
        #pipe result through findstr to remove warnings that are hard to suppress, and pipe through more to prevent error code from no matches aborting the compile
        COMMAND ${bisoncmdprefix} ${bisoncmd} --report=state --defines=${CMAKE_CURRENT_BINARY_DIR}/bisongram.h --output=${CMAKE_CURRENT_BINARY_DIR}/bisongram.cpp ${CMAKE_CURRENT_SOURCE_DIR}/bisongram.y 2>&1 | findstr /V "unused value" | more
        DEPENDS bisongram.y
    )
else()
    add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bisongram.cpp ${CMAKE_CURRENT_BINARY_DIR}/bisongram.h
        #pipe result through grep to remove warnings that are hard to suppress, and pipe through cat to prevent error code from no matches aborting the compile
        COMMAND ${bisoncmdprefix} ${bisoncmd} --report=state --defines=${CMAKE_CURRENT_BINARY_DIR}/bisongram.h --output=${CMAKE_CURRENT_BINARY_DIR}/bisongram.cpp ${CMAKE_CURRENT_SOURCE_DIR}/bisongram.y 2>&1 | grep -v "unused value" | cat
        DEPENDS bisongram.y
    )
endif()

add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bisonlex.cpp
    COMMAND ${flexcmdprefix} ${flexcmd} --outfile=${CMAKE_CURRENT_BINARY_DIR}/bisonlex.cpp --header-file=${CMAKE_CURRENT_BINARY_DIR}/bisonlex.hpp ${CMAKE_CURRENT_SOURCE_DIR}/bisonlex.l
    DEPENDS bisonlex.l
)

if (WIN32)
    set_source_files_properties (bisongram.cpp PROPERTIES COMPILE_FLAGS "/Od")
endif()

#ADD_DEFINITIONS( -D_USRDLL -DHQL_EXPORTS -DHQLFOLD_EXPORTS -DHQLTRANS_EXPORTS )

HPCC_ADD_LIBRARY( eclanalyser SHARED ${SRCS} ${CMAKE_CURRENT_BINARY_DIR}/bisongram.cpp ${CMAKE_CURRENT_BINARY_DIR}/bisonlex.cpp  )

install ( TARGETS eclanalyser RUNTIME DESTINATION ${EXEC_DIR} LIBRARY DESTINATION ${LIB_DIR} )
target_link_libraries ( eclanalyser
         jlib
         nbcd
         eclrtl
         deftype
         ${CPPUNIT_LIBRARIES}
    )
