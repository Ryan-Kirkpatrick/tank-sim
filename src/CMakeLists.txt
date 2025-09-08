# External Dependencies
add_subdirectory(deps)

# Main Project
set(NAME tank_sim_core.elf)

add_executable(${NAME} 
    freertos_hooks.c
    main.c

    config/config.c

    control/control_map.c
    control/hx710c.c
    control/input_task.c

    terminal/terminal.c

    usb_keyboard/keyboard_task.c
    usb_keyboard/usb_descriptors.c
    usb_keyboard/usb_task.c

    util/tank_assert.c

    ${FREERTOS_SRC}
)

target_include_directories(${NAME} 
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${FREERTOS_INC}
        ${TINYUSB_INC}
)

target_link_libraries(${NAME}
    PUBLIC 
        hardware_adc
        hardware_exception
        hardware_flash
        pico_stdlib
        pico_unique_id 
        tinyusb_board
        tinyusb_device 
)

pico_add_extra_outputs(${NAME})
pico_enable_stdio_usb(${NAME} 0)
pico_enable_stdio_uart(${NAME} 1)

message(STATUS "Binary will be at: ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2")