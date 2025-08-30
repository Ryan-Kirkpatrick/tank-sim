FetchContent_Declare(
  pico_sdk
  GIT_REPOSITORY https://github.com/raspberrypi/pico-sdk.git
  GIT_TAG 2.2.0
  GIT_SUBMODULES_RECURSE ON
)
FetchContent_MakeAvailable(pico_sdk)

pico_sdk_init()