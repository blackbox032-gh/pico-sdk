if (NOT TARGET pico_secure)
    pico_add_library(pico_secure)
    target_include_directories(pico_secure_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_sources(pico_secure INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/secure.c)

    pico_mirrored_target_link_libraries(pico_secure INTERFACE
            pico_bootrom)

    function(pico_set_security_defines SECURE_TARGET NONSECURE_TARGET)
        set(options
            ALLOW_NONSECURE_STDIO
            ALLOW_NONSECURE_RAND
            ALLOW_NONSECURE_DMA
            ALLOW_NONSECURE_USER_IRQ
            ALLOW_NONSECURE_PIO
            ALLOW_NONSECURE_GPIO
        )
        set(oneValueArgs
            ASSIGN_NONSECURE_TIMER
        )
        cmake_parse_arguments(PARSE_ARGV 2 OPTS "${options}" "${oneValueArgs}" "")

        target_compile_definitions(${SECURE_TARGET} PRIVATE
            PICO_SECURE=1
    
            # Stack guards are required
            PICO_USE_STACK_GUARDS=1
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
    
        foreach(arg IN LISTS options)
            if (OPTS_${arg})
                target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_${arg}=1)
                target_compile_definitions(${NONSECURE_TARGET} PRIVATE PICO_${arg}=1)
            endif()
        endforeach()

        if (OPTS_ASSIGN_NONSECURE_TIMER)
            target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_ASSIGN_NONSECURE_TIMER=${OPTS_ASSIGN_NONSECURE_TIMER})
            target_compile_definitions(${NONSECURE_TARGET} PRIVATE PICO_DEFAULT_TIMER=${OPTS_ASSIGN_NONSECURE_TIMER})
        endif()
    endfunction()
endif()



