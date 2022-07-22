cmake_minimum_required(VERSION 3.22)
project(poco_xml_to_kv)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set the POCO paths and libs
set(POCO_PREFIX "/usr/local") # the directory containing "include" and "lib"
set(POCO_INCLUDE_DIR "${POCO_PREFIX}/include")
set(POCO_LIB_DIR "${POCO_PREFIX}/lib")
set(POCO_LIBS
        "${POCO_LIB_DIR}/libPocoNet.dylib"
        "${POCO_LIB_DIR}/libPocoUtil.dylib"
        "${POCO_LIB_DIR}/libPocoFoundation.dylib"
#        "${POCO_LIB_DIR}/libPocoNetSSL.dylib"
        "${POCO_LIB_DIR}/libPocoXML.dylib")

link_directories(${POCO_LIB_DIR}) # 添加非标准的共享库搜索路径

add_executable(${PROJECT_NAME}
        main.cpp
        )

include_directories(${POCO_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${POCO_LIBS}) # 把目标文件与库文件进行链接