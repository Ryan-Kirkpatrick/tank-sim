set(FREERTOS_SRC
    ${CMAKE_CURRENT_LIST_DIR}/croutine.c
    ${CMAKE_CURRENT_LIST_DIR}/event_groups.c
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/list.c
    ${CMAKE_CURRENT_LIST_DIR}/queue.c
    ${CMAKE_CURRENT_LIST_DIR}/stream_buffer.c
    ${CMAKE_CURRENT_LIST_DIR}/tasks.c
    ${CMAKE_CURRENT_LIST_DIR}/timers.c

    ${CMAKE_CURRENT_LIST_DIR}/RP2040/port.c
    ${CMAKE_CURRENT_LIST_DIR}/RP2040/idle_task_static_memory.c

    PARENT_SCOPE
)

set(FREERTOS_INC
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/RP2040/include

    PARENT_SCOPE
)