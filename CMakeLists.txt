cmake_minimum_required(VERSION 3.27)
project(cs202_24_prj_sophie_zac)

set(CMAKE_CXX_STANDARD 14)

include_directories(provided/src)
include_directories(provided/tests/unit)

add_executable(cs202_24_prj_sophie_zac
        done/handouts/01.imgfs.md
        done/handouts/01.main.md
        done/src/error.c
        done/src/error.h
        done/src/http-test-server.c
        done/src/http_net.h
        done/src/http_prot.h
        done/src/image_content.h
        done/src/image_dedup.h
        done/src/imgfs.h
        done/src/imgfs_server_service.h
        done/src/imgfs_tools.c
        done/src/imgfscmd.c
        done/src/imgfscmd_functions.c
        done/src/imgfscmd_functions.h
        done/src/socket_layer.h
        done/src/util.c
        done/src/util.h
        done/tests/unit/test.h
        done/tests/unit/unit-test-imgfslist.c
        done/tests/unit/unit-test-imgfsstruct.c
        done/tests/unit/unit-test-imgfstools.c

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
        done/src/main.c
        done/tests/unit/unit-test-imgfslist.c
        done/tests/unit/unit-test-imgfsstruct.c

)
