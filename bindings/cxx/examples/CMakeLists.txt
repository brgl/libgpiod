set(EXAMPLES
        gpiod_cxx_tests
        gpiodetectcxx
        gpiofindcxx
        gpiogetcxx
        gpioinfocxx
        gpiomoncxx
        gpiosetcxx)

foreach (ex ${EXAMPLES})
    add_executable(${ex} ${ex}.cpp)
    target_link_libraries(${ex} gpiodcxx)
endforeach()
