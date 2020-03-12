
cmake_minimum_required(VERSION 3.4.1)

find_library( log-lib
              log )

set(distribution_DIR ../../../../libs)

add_library( native-lib
             SHARED
             src/main/cpp/FFmpeg-example.cpp
             src/main/cpp/native-lib.cpp)

add_library( avcodec-57
             SHARED
             IMPORTED)
set_target_properties( avcodec-57
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/armeabi-v7a/libavcodec-57.so)

add_library( avfilter-6
             SHARED
             IMPORTED)
set_target_properties( avfilter-6
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/armeabi-v7a/libavfilter-6.so)

add_library( avformat-57
             SHARED
             IMPORTED)
set_target_properties( avformat-57
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/armeabi-v7a/libavformat-57.so)

add_library( avutil-55
             SHARED
             IMPORTED)
set_target_properties( avutil-55
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/armeabi-v7a/libavutil-55.so)

add_library( swresample-2
             SHARED
             IMPORTED)
set_target_properties( swresample-2
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/armeabi-v7a/libswresample-2.so)

add_library( swscale-4
             SHARED
             IMPORTED)
set_target_properties( swscale-4
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/armeabi-v7a/libswscale-4.so)

add_library( libpostproc-54
             SHARED
             IMPORTED)
set_target_properties( libpostproc-54
                       PROPERTIES IMPORTED_LOCATION
                       ${distribution_DIR}/armeabi-v7a/libpostproc-54.so)

include_directories(libs/include)

target_link_libraries( native-lib
                       libpostproc-54
                       avcodec-57
                       avfilter-6
                       avformat-57
                       avutil-55
                       swresample-2
                       swscale-4
                       ${log-lib} )