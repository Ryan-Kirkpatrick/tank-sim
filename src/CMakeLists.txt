set(NAME tank_sim_core.elf)

add_subdirectory(pico_sdk)

add_subdirectory(freertos)


set(FREERTOS_PORT GCC_RP2040)


set(SOURCES
    main.c
    usb_descriptors.c
    ${FREERTOS_SRC}
)


add_executable(${NAME} ${SOURCES})

target_include_directories(${NAME} 
    PUBLIC
        ${FREERTOS_INC}
        tinyusb/config
)

target_link_libraries(${NAME}
    PUBLIC 
        pico_stdlib
        hardware_exception
        hardware_adc
        pico_unique_id 
        tinyusb_device 
        tinyusb_board
)

pico_add_extra_outputs(${NAME})