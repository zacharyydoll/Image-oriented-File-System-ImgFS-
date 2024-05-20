cmake_minimum_required(VERSION 3.27)
project(cs202_24_prj_sophie_zac)

set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_OSX_ARCHITECTURES arm64)

# Define the executable target
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
        done/image_content.c
        done/imgfs_create.c
        done/imgfs_delete.c
        done/image_dedup.c
        done/imgfs_insert.c
        done/imgfs_read.c
        done/http_net.c
        done/http_net.c
        provided/src/imgfs_server_service.c
        done/PersonalTests/test_http_prot.c
        done/tests/unit/unit-test-imgfsread.c
        done/tests/unit/personal_tests_week12.c
        done/imgfs_list.c
)

# Specify directories to include during the build process
target_include_directories(cs202_24_prj_sophie_zac PRIVATE
        provided/src
        provided/tests/unit
        /usr/include/glib-2.0
        /usr/local/opt/glib/include/glib-2.0
        /usr/local/opt/glib/lib/glib-2.0/include
        /usr/lib/x86_64-linux-gnu/glib-2.0/include
        /usr/local/opt/openssl@3/include
        /usr/local/opt/vips/include
)

# Specify the directories where the linker should look for libraries
target_link_directories(cs202_24_prj_sophie_zac PRIVATE
        /usr/local/opt/openssl@3/lib
        /usr/local/opt/glib/lib
        /usr/local/opt/vips/lib
)

# Link libraries
target_link_libraries(cs202_24_prj_sophie_zac
        ssl
        crypto
        glib-2.0
        vips.42
        vips-cpp.42)

#set_target_properties(cs202_24_prj_sophie_zac PROPERTIES
        #OSX_ARCHITECTURES arm64)
