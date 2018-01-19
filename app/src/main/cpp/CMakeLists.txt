cmake_minimum_required(VERSION 2.8)

set(base_dir ${CMAKE_CURRENT_LIST_DIR})
set(inc_dir ${base_dir})

set(lib_name "native-lib")

file(GLOB src_files
        ${base_dir}/*.cpp
)

include_directories(${inc_dir}
                    ${inc_dir}/include
)

add_library("ffmpeg" SHARED IMPORTED)
set_target_properties(ffmpeg PROPERTIES IMPORTED_LOCATION
${base_dir}/../../../libs/${ANDROID_ABI}/libijkffmpeg.so)


add_library(${lib_name} SHARED ${src_files})

target_link_libraries(${lib_name}
                        ffmpeg
                        log
                        android
)