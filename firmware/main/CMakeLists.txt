set(srcs "main.c"
         "board/battery.c"
         "board/servo.c"
         "board/buzzer.c"
         "board/light.c")

if(CONFIG_DORY_DEBUG_SERIAL)
    list(APPEND srcs "debug_serial.c")
endif()

if(CONFIG_DORY_WIRELESS)
    list(APPEND srcs "com.c")
endif()

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    REQUIRES dory_protocol)
