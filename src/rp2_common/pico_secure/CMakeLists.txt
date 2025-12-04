if (NOT TARGET pico_secure)
    pico_add_library(pico_secure)
    target_include_directories(pico_secure_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_sources(pico_secure INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/secure.c)

    pico_mirrored_target_link_libraries(pico_secure INTERFACE
            hardware_exception
            pico_bootrom)


    # pico_set_security_options(SECURE_TARGET NONSECURE_TARGET <OPTIONS>...)
    # \brief_nodesc\ Set matching security options for a secure and non-secure target
    #
    # Set matching security options for a secure and non-secure target, so they have a compatible set of features.
    #
    # Also sets PICO_SECURE=1 and PICO_NONSECURE=1 on the secure and non-secure targets respectively, along with
    # any other required defines (eg PICO_USE_STACK_GUARDS=1 on the secure target).
    #
    # The options are:
    # - STDIO: Allow non-secure to use secure stdio
    # - RAND: Allow non-secure to get random numbers
    # - DMA: Allow non-secure to request DMA channels
    # - USER_IRQ: Allow non-secure to request user IRQs
    # - PIO: Allow non-secure to request PIOs
    # - GPIO: Allow non-secure to access GPIOs assigned to non-secure (eg with gpio_assign_to_ns)
    # - USB: Allow non-secure to access USB
    # - RESETS: Allow non-secure to access resets specified by PICO_ALLOW_NONSECURE_RESETS_MASK (automatically set if USB is set)
    # - NONSECURE_TIMER <index>: Assign specified timer to non-secure
    #
    # \param\ SECURE_TARGET The secure target
    # \param\ NONSECURE_TARGET The non-secure target
    # \param\ OPTIONS The options to set
    function(pico_set_security_options SECURE_TARGET NONSECURE_TARGET)
        set(options STDIO RAND DMA USER_IRQ PIO GPIO USB RESETS)
        set(oneValueArgs NONSECURE_TIMER)
        cmake_parse_arguments(PARSE_ARGV 2 OPTS "${options}" "${oneValueArgs}" "")

        target_compile_definitions(${SECURE_TARGET} PRIVATE
            PICO_SECURE=1
    
            # Stack guards are required
            PICO_USE_STACK_GUARDS=1
        )
    
        target_compile_definitions(${NONSECURE_TARGET} PRIVATE
            PICO_NONSECURE=1
        )

        # Options that require resets
        if ((NOT OPTS_RESETS) AND OPTS_USB)
            set(OPTS_RESETS 1)
        endif()
    
        foreach(arg IN LISTS options)
            if (OPTS_${arg})
                target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_ALLOW_NONSECURE_${arg}=1)
                target_compile_definitions(${NONSECURE_TARGET} PRIVATE PICO_ALLOW_NONSECURE_${arg}=1)
            endif()
        endforeach()

        if (OPTS_NONSECURE_TIMER)
            target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_ASSIGN_NONSECURE_TIMER=${OPTS_NONSECURE_TIMER})
            target_compile_definitions(${NONSECURE_TARGET} PRIVATE PICO_DEFAULT_TIMER=${OPTS_NONSECURE_TIMER})
        endif()
    endfunction()

    # pico_set_security_ram_split(SECURE_TARGET NONSECURE_TARGET <OPTIONS>...)
    # \brief_nodesc\ Set ram split for a secure and non-secure target
    #
    # Set ram split for a secure and non-secure target, so they don't use the same memory.
    #
    # Each split option requires arguments to specify the memory sizes
    #
    # The split options are:
    # - SIMPLE <SECURE_LENGTH> <SECURE_SCRATCH_LENGTH>: Secure using start of main SRAM, NonSecure using end of main SRAM and scratch
    #
    # \param\ SECURE_TARGET The secure target
    # \param\ NONSECURE_TARGET The non-secure target
    # \param\ OPTIONS The options to set
    function(pico_set_security_ram_split SECURE_TARGET NONSECURE_TARGET)
        set(multiValueArgs SIMPLE)
        cmake_parse_arguments(PARSE_ARGV 2 OPTS "" "" "${multiValueArgs}")

        foreach(arg IN LISTS multiValueArgs)
            if (OPTS_${arg})
            endif()
        endforeach()

        if (OPTS_SIMPLE)
            message("OPTS_SIMPLE ${OPTS_SIMPLE}")
            list(GET OPTS_SIMPLE 0 SECURE_LENGTH)
            list(GET OPTS_SIMPLE 1 SECURE_SCRATCH_LENGTH)

            message("SECURE_LENGTH ${SECURE_LENGTH} SECURE_SCRATCH_LENGTH ${SECURE_SCRATCH_LENGTH}")

            pico_set_linker_script_var(${SECURE_TARGET} RAM_LENGTH "${SECURE_LENGTH}-(${SECURE_SCRATCH_LENGTH}*2)")
            pico_set_linker_script_var(${SECURE_TARGET} SCRATCH_X_ORIGIN "RAM_ORIGIN+RAM_LENGTH")
            pico_set_linker_script_var(${SECURE_TARGET} SCRATCH_X_LENGTH ${SECURE_SCRATCH_LENGTH})
            pico_set_linker_script_var(${SECURE_TARGET} SCRATCH_Y_ORIGIN "SCRATCH_X_ORIGIN+SCRATCH_X_LENGTH")
            pico_set_linker_script_var(${SECURE_TARGET} SCRATCH_Y_LENGTH SCRATCH_X_LENGTH)

            pico_set_linker_script_var(${NONSECURE_TARGET} RAM_ORIGIN "RAM_ORIGIN_DEFAULT+${SECURE_LENGTH}")
            pico_set_linker_script_var(${NONSECURE_TARGET} RAM_LENGTH "RAM_LENGTH_DEFAULT-${SECURE_LENGTH}")

            target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_SECURITY_SPLIT_SIMPLE=1 PICO_SECURITY_SPLIT_SIMPLE_SECURE_LENGTH=${SECURE_LENGTH})
        endif()

        target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_SECURITY_SPLIT_CONFIGURED=1)

    endfunction()
endif()
