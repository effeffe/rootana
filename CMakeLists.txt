cmake_minimum_required(VERSION 3.8)

project(rootana)

# set default definitions
set(HAVE_LIBZ 1)
set(HAVE_MIDAS 0)
set(HAVE_ROOT 0)
set(HAVE_ROOT_HTTP 0)
set(HAVE_ROOT_XML 0)
set(HAVE_THTTP_SERVER 0)
set(HAVE_XMLSERVER 0)
set(HAVE_LIBNETDIRECTORY 0)
set(HAVE_CXX11_THREADS 0)



find_package(Git REQUIRED)

# checkout git submodules
execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GIT_SUBMOD_RESULT
)



# Find MIDAS. Priority:
# - local cmake config: `cmake -DMIDASSYS=...`
# - global MIDAS: `export MIDASSYS=...`
# - provided by find_package
set(MIDASSYS "" CACHE PATH "")
if("${MIDASSYS}" STREQUAL "")
    set(MIDASSYS "$ENV{MIDASSYS}")
endif()
if("${MIDASSYS}" STREQUAL "")
    find_package(Midas QUIET)
else()
    set(MIDAS_FOUND TRUE)
    set(MIDAS_INCLUDE_DIRS "$ENV{MIDASSYS}/include")
    set(MIDAS_LIBRARIES "$ENV{MIDASSYS}/lib/libmidas.a" -lutil -lrt)
endif()
if(${MIDAS_FOUND})
    message(DEBUG "[${PROJECT_NAME}] MIDAS_INCLUDE_DIRS = ${MIDAS_INCLUDE_DIRS}")
    message(DEBUG "[${PROJECT_NAME}] MIDAS_LIBRARIES = ${MIDAS_LIBRARIES}")
endif()

find_package(ROOT COMPONENTS
    Gui # TG* objects
    RHTTP # THttpServer
    XMLIO XMLParser
)
if(${ROOT_FOUND})
    set(CMAKE_CXX_FLGAGS ${ROOT_CXX_FLAGS})
endif()



add_library(rootana STATIC)

set(ROOTANA_INCLUDE_DIRS
    ${MIDAS_INCLUDE_DIRS}
    ${ROOT_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}/include
    libAnalyzer
    libAnalyzerDisplay
    libMidasInterface
    libNetDirectory
    libMidasInterface
    libMidasServer
    libNetDirectory
    libUnpack
    libXmlServer
    midasio
    mjson
    mvodb
    mxml
)



# default sources that do not require MIDAS or ROOT
file(GLOB SOURCES
    libMidasInterface/TMidasEvent.cxx
    libMidasServer/midasServer.cxx
    libUnpack/*.cxx
    manalyzer/manalyzer.cxx
    midasio/*.cxx midasio/*.c
    mjson/*.cxx
    mvodb/mjsonodb.cxx mvodb/mvodb.cxx mvodb/mvodb.cxx mvodb/mxmlodb.cxx mvodb/nullodb.cxx
    mxml/*.cxx
)



if(${MIDAS_FOUND})
    set(HAVE_MIDAS 1)
    target_compile_definitions(rootana PUBLIC
        -DHAVE_MIDAS -DOS_LINUX -Dextname
    )
    file(GLOB SOURCES
        ${SOURCES}
        libMidasInterface/TMidasOnline.cxx
        mvodb/midasodb.cxx
    )
endif()

if(${ROOT_FOUND})
    set(HAVE_ROOT 1)
    set(HAVE_LIBNETDIRECTORY 1)
    set(HAVE_CXX11_THREADS 1)
    target_compile_definitions(rootana PUBLIC
        -DHAVE_ROOT -DHAVE_LIBNETDIRECTORY
    )
    file(GLOB SOURCES
        ${SOURCES}
        libAnalyzer/T*.cxx
        libAnalyzerDisplay/T*.cxx
        libNetDirectory/*.cxx
    )
endif()

if(${ROOT_http_FOUND})
    set(HAVE_ROOT_HTTP 1)
    set(HAVE_THTTP_SERVER 1)
    target_compile_definitions(rootana PUBLIC
        -DHAVE_ROOT_HTTP -DHAVE_THTTP_SERVER
    )
endif()

if(${ROOT_xml_FOUND})
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
message(DEBUG "[${PROJECT_NAME}] HAVE_MIDAS = ${HAVE_MIDAS}")
message(DEBUG "[${PROJECT_NAME}] HAVE_ROOT = ${HAVE_ROOT}")
message(DEBUG "[${PROJECT_NAME}] HAVE_ROOT_HTTP = ${HAVE_ROOT_HTTP}")
message(DEBUG "[${PROJECT_NAME}] HAVE_ROOT_XML = ${HAVE_ROOT_XML}")
message(DEBUG "[${PROJECT_NAME}] HAVE_THTTP_SERVER = ${HAVE_THTTP_SERVER}")
message(DEBUG "[${PROJECT_NAME}] HAVE_XMLSERVER = ${HAVE_XMLSERVER}")
message(DEBUG "[${PROJECT_NAME}] HAVE_LIBNETDIRECTORY = ${HAVE_LIBNETDIRECTORY}")
message(DEBUG "[${PROJECT_NAME}] HAVE_CXX11_THREADS = ${HAVE_CXX11_THREADS}")



configure_file(rootana_config.h.in include/rootana_config.h @ONLY)



# generate ROOT DICTs
if(${ROOT_FOUND})
    # root_generate_dictionary does not allow setting include dirs
    include_directories(
        ${ROOTANA_INCLUDE_DIRS}
    )
    root_generate_dictionary(TFancyHistogramCanvasDict
        ${CMAKE_CURRENT_SOURCE_DIR}/libAnalyzerDisplay/TFancyHistogramCanvas.hxx
        LINKDEF libAnalyzerDisplay/TFancyHistogramCanvas_LinkDef.h
    )
    root_generate_dictionary(TMainDisplayWindowDict
        ${CMAKE_CURRENT_SOURCE_DIR}/libAnalyzerDisplay/TMainDisplayWindow.hxx
        LINKDEF libAnalyzerDisplay/TMainDisplayWindow_LinkDef.h
    )
    root_generate_dictionary(TRootanaDisplayDict
        ${CMAKE_CURRENT_SOURCE_DIR}/libAnalyzerDisplay/TRootanaDisplay.hxx
        LINKDEF libAnalyzerDisplay/TRootanaDisplay_LinkDef.h
    )
    root_generate_dictionary(TNetDirectoryDict
        ${CMAKE_CURRENT_SOURCE_DIR}/libNetDirectory/TNetDirectory.h
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
    ${MIDAS_LIBRARIES}
    ${ROOT_LIBRARIES}
    -lpthread
)



get_directory_property(PARENT_DIRECTORY PARENT_DIRECTORY)
if(${PARENT_DIRECTORY})
    add_subdirectory(libAnalyzer)
    add_subdirectory(libAnalyzerDisplay)
    add_subdirectory(libMidasInterface)
    add_subdirectory(libMidasServer)
    add_subdirectory(manalyzer)
    add_subdirectory(old_analyzer)
endif()
