add_library(gpiodcxx
    chip.cpp
    iter.cpp
    line.cpp
    line_bulk.cpp)

target_include_directories(gpiodcxx
    PRIVATE
        .
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/gpiod>)

target_compile_options(gpiodcxx
    PRIVATE
        -Wall -Wextra
        -fvisibility=hidden)

target_compile_features(gpiodcxx
    PUBLIC
        cxx_std_11)

set_target_properties(gpiodcxx
    PROPERTIES
        VERSION ${ABI_CXX_VERSION}
        SOVERSION ${ABI_CXX_SOVERSION}
        PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/gpiod.hpp)

target_link_libraries(gpiodcxx
    PUBLIC
        gpiod)

install(TARGETS gpiodcxx EXPORT libgpiodCXXConfig
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gpiod)
install(EXPORT libgpiodCXXConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)

add_subdirectory(examples)
