cmake_minimum_required(VERSION 3.8)
project(gholy)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_custom_target(holyhttp COMMAND make)
include_directories(.)

set(SOURCE_FILES
        cgi/cgi.h
        cgi/init.c
        cgi/test.c
        server/connection.c
        server/connection.h
        server/packet.c
        server/packet.h
        server/request.c
        server/request.h
        server/server.c
        server/server.h
        server/session.h
        server/template.c
        server/template.h
        server/timer.c
        server/timer.h
        utils/address.h
        utils/dataset.c
        utils/dataset.h
        utils/dict.c
        utils/dict.h
        utils/file.c
        utils/file.h
        utils/list.h
        utils/print.c
        utils/print.h
        utils/rbtree.c
        utils/rbtree.h
        utils/string.c
        utils/string.h
        utils/time.c
        utils/time.h
        utils/tlv.c
        utils/tlv.h
        utils/types.h
        .gitignore
        alias.mk
        common.mk
        config.h
        main.c
        Makefile
        test.c)

add_executable(gholy ${SOURCE_FILES})
