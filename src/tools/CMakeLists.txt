add_library(tools-common STATIC tools-common.c)

target_compile_options(tools-common
    PUBLIC
        -Wall -Wextra)

target_compile_definitions(tools-common
    PUBLIC
        -D_GNU_SOURCE)

target_link_libraries(tools-common
    PUBLIC
        gpiod)

set(TOOLS gpiodetect gpioinfo gpioget gpioset gpiomon gpiofind)

foreach(tool ${TOOLS})
    add_executable(${tool} ${tool}.c)
    target_link_libraries(${tool} tools-common)
endforeach()

install(TARGETS ${TOOLS} RUNTIME DESTINATION bin)
