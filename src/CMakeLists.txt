# External Dependencies
add_subdirectory(deps)

# Main Project
set(NAME tank_sim_core.elf)

add_executable(${NAME} 
    main.c
    reporter.c
    usb.c
    usb_descriptors.c
    ${FREERTOS_SRC}
)

target_include_directories(${NAME} 
    PUBLIC
        ${FREERTOS_INC}
        ${TINYUSB_INC}
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