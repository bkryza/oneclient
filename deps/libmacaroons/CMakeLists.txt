add_library(libmacaroons STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/base64.c
    ${CMAKE_CURRENT_SOURCE_DIR}/macaroons.c
    ${CMAKE_CURRENT_SOURCE_DIR}/packet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/port.c)

target_include_directories(libmacaroons PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../helpers/etls/c_src/deps/boringssl/include  ${CMAKE_CURRENT_SOURCE_DIR})
