#include "pico/stdlib.h"
#include "hardware/uart.h"

#define BIT(n)  (1u<<(n))
#define LED_INITAL_MS 500
#define LED_MIN_MS 50
#define LED_MAX_MS 60000
#define LED_CHANGE_STEP_MS 50

#define LED_PIN_1 2
#define LED_PIN_2 3 
#define LED_PIN_3 4 
#define LED_PIN_4 5 
#define LED_PIN_5 6 
#define LED_PIN_6 7 
#define LED_PIN_7 8 
#define LED_PIN_8 9 
#define LED_PIN_9 10 
#define BUTTON_PIN 16

#define BUTTON_IRQ_TIMEOUT_MS 30

#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

uint8_t led_pins[] = {LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4, LED_PIN_5, LED_PIN_6, LED_PIN_7, LED_PIN_8, LED_PIN_9};
uint32_t led_init_bitmask = BIT(LED_PIN_1) | BIT(LED_PIN_2) | BIT(LED_PIN_3) | BIT(LED_PIN_4) | BIT(LED_PIN_5) | BIT(LED_PIN_6) | BIT(LED_PIN_7) | BIT(LED_PIN_8) | BIT(LED_PIN_9);

volatile char direction_forward = true;
volatile int32_t led_ms = LED_INITAL_MS;

const uint8_t speed_incr_char = '+';
const uint8_t speed_decr_char = '-';

/* Function prototypes */
int64_t alarm_handler(alarm_id_t id, __unused void *user_data);
void button_callback(uint gpio, __unused uint32_t events);
void uart_rx_handler();
void sys_init();

void sys_init()
{
    stdio_init_all();

    gpio_init_mask(led_init_bitmask);
    gpio_set_dir_out_masked(led_init_bitmask);

    // button initialisation with an internal pull-up resistor, the direction in gpio_init by default is GPIO_IN
    gpio_init(BUTTON_PIN);
    gpio_pull_up(BUTTON_PIN);

    // enable interrupt on the button press
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    // UART inits
    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    uart_set_fifo_enabled(UART_ID, true);

    irq_set_exclusive_handler(UART0_IRQ, uart_rx_handler);
    irq_set_enabled(UART0_IRQ, true);

    // set this to RX only
    uart_set_irq_enables(UART_ID, true, false);
}

int main()
{
    sys_init();

    uint32_t led_index = 0;
    // set the first LED to "on" since the following logic sets the next LEDs to on/off accordingly, skipping the first one
    gpio_put(led_pins[led_index], true);
    sleep_ms(led_ms);

    while (true) {

        // turn the current LED off, get ready for the next LED to be turned on
        gpio_put(led_pins[led_index], false);

        if (direction_forward == true)
        {
            /* Edge case for going forward - last LED */
            if (led_index == (sizeof(led_pins) - 1))
            {
                led_index = 0;
            }
            else
            {
                led_index++;
            }
        }
        else
        {
            /* Edge case when going backwards - first LED */
            if (led_index == 0)
            {
                led_index = sizeof(led_pins) - 1;
            }
            else
            {
                led_index--;
            }
        }

        /* Set only the LED at the specified index */
        gpio_put(led_pins[led_index], true);
        sleep_ms(led_ms);
    }

    return 0;
}

/* Interrupt handlers */
int64_t alarm_handler(alarm_id_t id, __unused void *user_data)
{
    gpio_set_irq_enabled(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);
    return 0;
}

void button_callback(uint gpio, __unused uint32_t events)
{
    direction_forward = !direction_forward;
    /* Disable the button pin interrupt and wait for the alarm to set it up again after a certain timeout defined in BUTTON_IRQ_TIMEOUT_MS
    This shall serve (in addition to hardware button debouncing) as a way to change the light direction during the sleep_ms phase for the LEDs */
    add_alarm_in_ms(BUTTON_IRQ_TIMEOUT_MS, alarm_handler, NULL, false);
    gpio_set_irq_enabled(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, false);
}

void uart_rx_handler()
{
    while (uart_is_readable(UART_ID)) {
        
        uint8_t ch = uart_getc(UART_ID);

        if (ch == speed_incr_char)
        {
            if ((led_ms - LED_CHANGE_STEP_MS) > LED_MIN_MS)
            {
                led_ms -= LED_CHANGE_STEP_MS;
                uart_puts(UART_ID, "\nSpeed increased\n");

            }
        }
        else if (ch == speed_decr_char)
        {
            if ((led_ms + LED_CHANGE_STEP_MS) < LED_MAX_MS)
            {
                led_ms += LED_CHANGE_STEP_MS;
                uart_puts(UART_ID, "\nSpeed decreased\n");
            }

        }
        else
        {
            /* Invalid character, no change*/
            uart_puts(UART_ID, "\nInvalid character\n");

            continue;
        }
    }
}