#include "debug_serial.h"

#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board/battery.h"
#include "board/buzzer.h"
#include "board/light.h"
#include "board/servo.h"


static void debug_serial_print_help(void)
{
    printf("\nDebug serial commands:\n");
    printf("  h - help\n");
    printf("  b - read battery voltage\n");
    printf("  l - toggle light\n");
    printf("  p - toggle servo 5V power\n");
    printf("  z - buzzer tone test\n");
    printf("  o - buzzer off\n");
    printf("  s - starting beeps\n");
    printf("  a - start leak alarm\n");
    printf("  q - stop leak alarm\n");
}

static void debug_serial_task(void *arg)
{
    (void) arg;

    bool light_on = false;
    bool power_on = false;

    printf("\nDory debug serial enabled\n");
    debug_serial_print_help();

    while (1) {
        int command = getchar();

        switch (command) {
        case 'h':
        case '?':
            debug_serial_print_help();
            break;
        case 'b':
            printf("Battery: %.2f V\n", battery_voltage_read());
            break;
        case 'l':
            light_on = !light_on;
            light_set(light_on);
            printf("Light: %s\n", light_on ? "on" : "off");
            break;
        case 'p':
            power_on = !power_on;
            set_power(power_on);
            printf("Servo 5V power: %s\n", power_on ? "on" : "off");
            break;
        case 'z':
            buzzer_tone(4000);
            printf("Buzzer tone on\n");
            break;
        case 'o':
            buzzer_off();
            printf("Buzzer off\n");
            break;
        case 's':
            printf("Starting beeps\n");
            buzzer_play_starting_beeps();
            break;
        case 'a':
            buzzer_start_leak_alarm();
            printf("Leak alarm started\n");
            break;
        case 'q':
            buzzer_stop_leak_alarm();
            printf("Leak alarm stopped\n");
            break;
        default:
            printf("Unknown command: %c\n", command);
            debug_serial_print_help();
            break;
        }
    }
}

void debug_serial_start(void)
{
    xTaskCreate(debug_serial_task, "debug_serial", 512, NULL, 5, NULL);
}
