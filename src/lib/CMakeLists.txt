add_library(gpiod core.c ctxless.c helpers.c iter.c misc.c)

target_compile_options(gpiod
    PRIVATE
        -Wall -Wextra
        -fvisibility=hidden)
    # -include ${PROJECT_BINARY_DIR}/config.h) is this really needed in 2019?
target_compile_definitions(gpiod
    PRIVATE
        -DGPIOD_VERSION_STR="${PROJECT_VERSION}"
        -D_GNU_SOURCE)

target_include_directories(gpiod
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/gpiod>)

set_target_properties(gpiod
    PROPERTIES
        VERSION ${ABI_VERSION}
        SOVERSION ${ABI_SOVERSION}
        PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/include/gpiod.h)

install(TARGETS gpiod EXPORT libgpiodConfig
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gpiod)
install(EXPORT libgpiodConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)
