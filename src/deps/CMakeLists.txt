add_subdirectory(pico_sdk)

add_subdirectory(freertos)
SET(FREERTOS_SRC ${FREERTOS_SRC} PARENT_SCOPE)
SET(FREERTOS_INC ${FREERTOS_INC} PARENT_SCOPE)

add_subdirectory(tinyusb)
SET(TINYUSB_INC ${TINYUSB_INC} PARENT_SCOPE)