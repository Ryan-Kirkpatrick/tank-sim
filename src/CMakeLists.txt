# External Dependencies
add_subdirectory(deps)

# Main Project
set(NAME tank_sim_core.elf)

add_executable(${NAME} 
    hx710c.c
    input_task.c
    keyboard_task.c
    main.c
    usb_descriptors.c
    usb_task.c
    tank_assert.c
    freertos_hooks.c

    ${FREERTOS_SRC}
)

target_include_directories(${NAME} 
    PUBLIC
        ${FREERTOS_INC}
        ${TINYUSB_INC}
)

target_link_libraries(${NAME}
    PUBLIC 
        hardware_adc
        hardware_exception
        pico_stdlib
        pico_unique_id 
        tinyusb_board
        tinyusb_device 
)

pico_add_extra_outputs(${NAME})
pico_enable_stdio_usb(${NAME} 0)
pico_enable_stdio_uart(${NAME} 1)

message(STATUS "Binary will be at: ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2")