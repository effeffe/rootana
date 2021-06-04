cmake_minimum_required(VERSION 3.8)

project("rootana"
        VERSION 1
        DESCRIPTION "ROOT analyzer for MIDAS data"
        HOMEPAGE_URL "https://midas.triumf.ca")

# set default definitions
set(HAVE_LIBZ 0)
set(HAVE_ROOT 0)
set(HAVE_ROOT_HTTP 0)
set(HAVE_ROOT_XML 0)
set(HAVE_THTTP_SERVER 0)
set(HAVE_XMLSERVER 0)
set(HAVE_LIBNETDIRECTORY 0)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
   # in standalone mode, install under <midas>/bin
   set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "" FORCE)
else()
   # there is an user choice or a higher level project
   # specifying the install path - keep that
   set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
endif()
message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

find_package(Git REQUIRED)

# checkout git submodules
execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GIT_SUBMOD_RESULT
)

find_package(ZLIB)

#
# Find MIDAS
#

if (NO_MIDAS)
   message(STATUS "${PROJECT_NAME}: MIDAS support is disabled via NO_MIDAS")
   unset(MIDAS_FOUND)
elseif(TARGET midas)
   message(STATUS "${PROJECT_NAME}: Building as a subproject of MIDAS")
   set(MIDAS_FOUND true)
elseif(DEFINED ENV{MIDASSYS})
   include($ENV{MIDASSYS}/lib/midas-targets.cmake)
   include($ENV{MIDASSYS}/lib/manalyzer-targets.cmake)
   message(STATUS "${PROJECT_NAME}: Building with MIDAS from $ENV{MIDASSYS}/lib/midas-targets.cmake")
   set(MIDAS_FOUND TRUE)
else()
   message(STATUS "${PROJECT_NAME}: Building without MIDAS")
   unset(MIDAS_FOUND)
endif()

#if(MIDASSYS STREQUAL "")
#    find_package(Midas)
#else()
#    set(MIDAS_FOUND TRUE)
#    set(MIDAS_INCLUDE_DIRS "$ENV{MIDASSYS}/include")
#    set(MIDAS_LIBRARIES "$ENV{MIDASSYS}/lib/libmidas.a" -lutil -lrt)
#endif()

#
# Find ROOT
#

find_package(ROOT COMPONENTS
    Gui # TG* objects
    RHTTP # THttpServer
    XMLIO XMLParser
)
if(ROOT_FOUND)
    # set c++ standard to one used by ROOT
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS} -DHAVE_ROOT")
endif()
if(ROOT_FOUND AND NOT COMMAND root_generate_dictionary)
    message(WARN "[${PROJECT_NAME}] root_generate_dictionary not found, try to include ROOTUseFile.cmake")
    include("${ROOT_USE_FILE}")
endif()



add_library(rootana STATIC)

set(ROOTANA_INCLUDE_DIRS
    ${MIDAS_INCLUDE_DIRS}
    ${ROOT_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libAnalyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/libAnalyzerDisplay
    ${CMAKE_CURRENT_SOURCE_DIR}/libMidasInterface
    ${CMAKE_CURRENT_SOURCE_DIR}/libNetDirectory
    ${CMAKE_CURRENT_SOURCE_DIR}/libMidasInterface
    ${CMAKE_CURRENT_SOURCE_DIR}/libMidasServer
    ${CMAKE_CURRENT_SOURCE_DIR}/libNetDirectory
    ${CMAKE_CURRENT_SOURCE_DIR}/libUnpack
    ${CMAKE_CURRENT_SOURCE_DIR}/libXmlServer
)

# default sources that do not require MIDAS or ROOT
file(GLOB SOURCES
    libMidasInterface/TMidasEvent.cxx
    libUnpack/*.cxx
)

if(ZLIB_FOUND)
    # TODO: rename HAVE_LIBZ to HAVE_ZLIB
    # (original rename was introduced in midasio commit f185f999
    # for compatibility with autotools which is not used)
    set(HAVE_LIBZ 1)
    target_compile_definitions(rootana PUBLIC
        -DHAVE_LIBZ
    )
endif()

if(MIDAS_FOUND)
    target_compile_definitions(rootana PUBLIC -DHAVE_MIDAS -DHAVE_TMFE)
    target_link_libraries(rootana PUBLIC midas)
    file(GLOB SOURCES
        ${SOURCES}
        libMidasInterface/TMidasOnline.cxx
        #mvodb/midasodb.cxx
    )
    get_target_property(MIDAS_INCLUDE_DIRS midas INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "${PROJECT_NAME}: MIDAS include path: ${MIDAS_INCLUDE_DIRS}")
else(MIDAS_FOUND)
    file(GLOB ROOTANA_INCLUDE_DIRS
    ${ROOTANA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/midasio
    ${CMAKE_CURRENT_SOURCE_DIR}/mjson
    ${CMAKE_CURRENT_SOURCE_DIR}/mvodb
    ${CMAKE_CURRENT_SOURCE_DIR}/mxml
    )
    file(GLOB SOURCES
    ${SOURCES}
    midasio/midasio.cxx
    midasio/*.c
    mjson/*.cxx
    mvodb/mjsonodb.cxx mvodb/mvodb.cxx mvodb/mvodb.cxx mvodb/mxmlodb.cxx mvodb/nullodb.cxx
    mxml/*.cxx
    )
endif(MIDAS_FOUND)

if(ROOT_FOUND)
    set(HAVE_ROOT 1)
    set(HAVE_LIBNETDIRECTORY 1)
    target_compile_definitions(rootana PUBLIC
        -DHAVE_ROOT -DHAVE_LIBNETDIRECTORY
    )
    file(GLOB SOURCES
        ${SOURCES}
        libAnalyzer/T*.cxx
        libAnalyzerDisplay/T*.cxx
        libMidasServer/midasServer.cxx
        libNetDirectory/*.cxx
    )
endif()

if(ROOT_http_FOUND)
    set(HAVE_ROOT_HTTP 1)
    set(HAVE_THTTP_SERVER 1)
    target_compile_definitions(rootana PUBLIC
        -DHAVE_ROOT_HTTP -DHAVE_THTTP_SERVER
    )
endif()

if(ROOT_xml_FOUND)
    set(HAVE_ROOT_XML 1)
    set(HAVE_XMLSERVER 1)
    file(GLOB SOURCES
        ${SOURCES}
        libXmlServer/*.cxx
    )
    target_compile_definitions(rootana PUBLIC
        -DHAVE_ROOT_XML
    )
endif()

message(DEBUG "[${PROJECT_NAME}] HAVE_LIBZ = ${HAVE_LIBZ}")
message(DEBUG "[${PROJECT_NAME}] HAVE_ROOT = ${HAVE_ROOT}")
message(DEBUG "[${PROJECT_NAME}] HAVE_ROOT_HTTP = ${HAVE_ROOT_HTTP}")
message(DEBUG "[${PROJECT_NAME}] HAVE_ROOT_XML = ${HAVE_ROOT_XML}")
message(DEBUG "[${PROJECT_NAME}] HAVE_THTTP_SERVER = ${HAVE_THTTP_SERVER}")
message(DEBUG "[${PROJECT_NAME}] HAVE_XMLSERVER = ${HAVE_XMLSERVER}")
message(DEBUG "[${PROJECT_NAME}] HAVE_LIBNETDIRECTORY = ${HAVE_LIBNETDIRECTORY}")
message(DEBUG "[${PROJECT_NAME}] ROOT_FOUND = ${ROOT_FOUND}")
message(DEBUG "[${PROJECT_NAME}] MIDAS_FOUND = ${MIDAS_FOUND}")

configure_file(rootana_config.h.in include/rootana_config.h @ONLY)

configure_file(
    ${PROJECT_SOURCE_DIR}/rootana-config.cmake.in
    ${PROJECT_SOURCE_DIR}/rootana-config.cmake
)



# generate ROOT DICTs
if(ROOT_FOUND)
    # root_generate_dictionary does not allow setting include dirs
    include_directories(
        ${ROOTANA_INCLUDE_DIRS}
        ${MIDAS_INCLUDE_DIRS}
    )
    root_generate_dictionary(TFancyHistogramCanvasDict
        TFancyHistogramCanvas.hxx
        LINKDEF libAnalyzerDisplay/TFancyHistogramCanvas_LinkDef.h
    )
    root_generate_dictionary(TMainDisplayWindowDict
        TMainDisplayWindow.hxx
        LINKDEF libAnalyzerDisplay/TMainDisplayWindow_LinkDef.h
    )
    root_generate_dictionary(TRootanaDisplayDict
        TRootanaDisplay.hxx
        LINKDEF libAnalyzerDisplay/TRootanaDisplay_LinkDef.h
    )
    root_generate_dictionary(TNetDirectoryDict
        TNetDirectory.h
        LINKDEF libNetDirectory/TNetDirectory_LinkDef.h
    )
    target_sources(rootana PRIVATE
        TFancyHistogramCanvasDict.cxx
        TMainDisplayWindowDict.cxx
        TRootanaDisplayDict.cxx
        TNetDirectoryDict.cxx
    )
endif()

target_sources(rootana PRIVATE
    ${SOURCES}
)

target_include_directories(rootana PUBLIC
    ${ROOTANA_INCLUDE_DIRS}
)

target_link_libraries(rootana PUBLIC
    ${ROOT_LIBRARIES}
    ${ZLIB_LIBRARIES}
    -lpthread
)

if(TARGET manalyzer)
   message(STATUS "${PROJECT_NAME}: Using manalyzer from MIDAS")
else(TARGET manalyzer)
   add_subdirectory(manalyzer)
endif(TARGET manalyzer)

get_directory_property(PARENT_DIRECTORY PARENT_DIRECTORY)
if(NOT PARENT_DIRECTORY)
    add_subdirectory(libAnalyzer)
    add_subdirectory(libAnalyzerDisplay)
    add_subdirectory(libMidasInterface)
    add_subdirectory(libMidasServer)
    add_subdirectory(old_analyzer)
endif()


# Only set install path to environment variable ROOTANASYS if is default, ROOTANASYS is set AND 
# you are exclusively building rootana (ie rootana isn't submodule to something else)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND DEFINED ENV{ROOTANASYS} AND ${CMAKE_PROJECT_NAME} MATCHES rootana)
    set(CMAKE_INSTALL_PREFIX "$ENV{ROOTANASYS}" CACHE PATH "" FORCE)
endif()
install(
    TARGETS event_dump event_skim
    DESTINATION bin
)
install(
    TARGETS rootana
    DESTINATION lib
)

file(GLOB HEADERS
    LIST_DIRECTORIES false
    ${CMAKE_CURRENT_BINARY_DIR}/include/rootana_config.h
    libAnalyzer/*.h*
    libAnalyzerDisplay/*.h*
    libMidasInterface/*.h*
    libNetDirectory/*.h*
    libMidasInterface/*.h*
    libMidasServer/*.h*
    libNetDirectory/*.h*
    libUnpack/*.h*
    libXmlServer/*.h*
    #manalyzer/*.h
    #midasio/*.h
    #mjson/*.h
    #mvodb/*.h
    #mxml/*.h
)

install(
    FILES ${HEADERS}
    DESTINATION include
)

#end
