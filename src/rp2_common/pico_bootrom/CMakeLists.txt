pico_add_library(pico_bootrom_headers NOFLAG)

target_include_directories(pico_bootrom_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

add_library(pico_bootrom INTERFACE)
target_sources(pico_bootrom INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/bootrom.c
        ${CMAKE_CURRENT_LIST_DIR}/bootrom_lock.c
        )

target_link_libraries(pico_bootrom_headers INTERFACE boot_picoboot_headers boot_bootrom_headers)
pico_mirrored_target_link_libraries(pico_bootrom INTERFACE pico_base hardware_boot_lock pico_flash hardware_dma hardware_irq pico_stdio hardware_pio)

# bootrom.c includes boot/picobin.h
# bootrom_lock.c includes pico/runtime_init.h
target_link_libraries(pico_bootrom INTERFACE boot_picobin_headers pico_runtime_init_headers)

function(pico_set_security_defines SECURE_TARGET NONSECURE_TARGET)
    target_compile_definitions(${SECURE_TARGET} PRIVATE
        PICO_SECURE=1
    )

    target_compile_definitions(${NONSECURE_TARGET} PRIVATE
        PICO_NONSECURE=1

        # These all require secure access
        PICO_RUNTIME_SKIP_INIT_BOOTROM_RESET=1
        PICO_RUNTIME_SKIP_INIT_PER_CORE_BOOTROM_RESET=1
        PICO_RUNTIME_SKIP_INIT_EARLY_RESETS=1
        PICO_RUNTIME_SKIP_INIT_BOOT_LOCKS_RESET=1
        PICO_RUNTIME_SKIP_INIT_BOOTROM_LOCKING_ENABLE=1
        PICO_RUNTIME_SKIP_INIT_POST_CLOCK_RESETS=1

        # This will have been done by secure anyway
        PICO_RUNTIME_SKIP_INIT_USB_POWER_DOWN=1
    )

    foreach(arg IN LISTS ARGN)
        target_compile_definitions(${SECURE_TARGET} PRIVATE ${arg})
        target_compile_definitions(${NONSECURE_TARGET} PRIVATE ${arg})
    endforeach()
endfunction()