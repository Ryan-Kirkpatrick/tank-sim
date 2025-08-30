set(NAME tank_sim_core.elf)

add_subdirectory(pico_sdk)

add_subdirectory(freertos)


set(FREERTOS_PORT GCC_RP2040)


set(SOURCES
    main.c

    ${FREERTOS_SRC}

)


add_executable(${NAME} ${SOURCES})

target_include_directories(${NAME} 
    PUBLIC
        ${FREERTOS_INC}
)

target_link_libraries(${NAME}
    PUBLIC 
        pico_stdlib
        hardware_exception
)

pico_add_extra_outputs(${NAME})