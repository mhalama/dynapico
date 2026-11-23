cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # compile_commands.json pro editor (include cesty SDK)
include(pico_sdk_import.cmake)   # zkopirovat z $PICO_SDK_PATH/external/

project(dynapico C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

# WiFi udaje: -DWIFI_SSID=... -DWIFI_PASSWORD=... nebo stejnojmenne promenne prostredi
foreach(v WIFI_SSID WIFI_PASSWORD)
    if(NOT DEFINED ${v} AND DEFINED ENV{${v}})
        set(${v} $ENV{${v}})
    endif()
endforeach()

add_executable(dynapico src/main.c src/tcp_server.c src/dyno.c)

target_include_directories(dynapico PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_compile_definitions(dynapico PRIVATE
    WIFI_SSID="${WIFI_SSID}"
    WIFI_PASSWORD="${WIFI_PASSWORD}")
target_compile_options(dynapico PRIVATE -Wall -Wextra)

# Funkce z DYNO_EXPORT(...) v dyno_api.h nesmi odstranit --gc-sections (gcc pro ARM nezna 'retain')
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS src/dyno_api.h)
file(STRINGS src/dyno_api.h DYNO_EXPORTS REGEX "^DYNO_EXPORT\\(")
foreach(line ${DYNO_EXPORTS})
    string(REGEX REPLACE "^DYNO_EXPORT\\(([A-Za-z0-9_]+)\\).*" "\\1" fn "${line}")
    target_link_options(dynapico PRIVATE "LINKER:--undefined=${fn}")
endforeach()

target_link_libraries(dynapico pico_stdlib pico_multicore pico_cyw43_arch_lwip_threadsafe_background)

pico_enable_stdio_usb(dynapico 1)
pico_enable_stdio_uart(dynapico 0)
pico_add_extra_outputs(dynapico)
