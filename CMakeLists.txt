cmake_minimum_required(VERSION 3.27)
project(cs202_24_prj_sophie_zac)

set(CMAKE_CXX_STANDARD 14)

include_directories(provided/src)
include_directories(provided/tests/unit)
include_directories(/usr/include/glib-2.0)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)

add_executable(cs202_24_prj_sophie_zac

        done/error.c
        done/error.h
        done/http_net.h
        done/http_prot.h
        done/image_content.h
        done/image_dedup.h
        done/imgfs.h
        done/imgfs_server_service.h
        done/imgfs_tools.c
        done/imgfscmd.c
        done/imgfscmd_functions.c
        done/imgfscmd_functions.h
        done/socket_layer.h
        done/util.c
        done/util.h


        provided/handouts/01.imgfs.md
        provided/handouts/01.main.md
        provided/src/error.c
        provided/src/error.h
        provided/src/http-test-server.c
        provided/src/http_net.h
        provided/src/http_prot.h
        provided/src/image_content.h
        provided/src/image_dedup.h
        provided/src/imgfs.h
        provided/src/imgfs_server_service.h
        provided/src/imgfs_tools.c
        provided/src/imgfscmd.c
        provided/src/imgfscmd_functions.c
        provided/src/imgfscmd_functions.h
        provided/src/socket_layer.h
        provided/src/util.c
        provided/src/util.h
        provided/tests/unit/test.h
        provided/tests/unit/unit-test-imgfslist.c
        provided/tests/unit/unit-test-imgfsstruct.c
        provided/tests/unit/unit-test-imgfstools.c
        done/image_content.c
        done/imgfs_create.c
        done/imgfs_delete.c


)
