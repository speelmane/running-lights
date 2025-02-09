#include "pico/time.h"
#include "hardware/address_mapped.h"
#include "hardware/irq.h"
#include "hardware/regs/intctrl.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/regs/sio.h"
#include "hardware/uart.h"

#define LED_INITAL_MS 100
#define LED_MIN_MS 10
#define LED_MAX_MS 1000
#define LED_CHANGE_STEP_MS 10

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
#define BUTTON_IRQ_TIMEOUT_MS 70

#define PADS_BANK0_GPIO (PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET)
#define IO_BANK0_GPIO (IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET)
#define GPIO_FUNC_UART 2
#define GPIO_FUNC_SIO 5
#define GPIO_REG_SIZE sizeof(io_rw_32)

#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

/* Some helpful macros */
#define BIT(n)  (1u<<(n))

/* UART debug definition, do nothing if DEBUG is not defined */
#ifdef DEBUG
#define DEBUG_UART(x) uart_puts(UART_ID, x);
#else
#define DEBUG_UART(x)
#endif

uint8_t led_pins[] = {LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4, LED_PIN_5, LED_PIN_6, LED_PIN_7, LED_PIN_8, LED_PIN_9};
uint32_t led_init_bitmask = BIT(LED_PIN_1) | BIT(LED_PIN_2) | BIT(LED_PIN_3) | BIT(LED_PIN_4) | BIT(LED_PIN_5) | BIT(LED_PIN_6) | BIT(LED_PIN_7) | BIT(LED_PIN_8) | BIT(LED_PIN_9);

volatile char direction_forward = true;
volatile int32_t led_ms = LED_INITAL_MS;

/* Characters used in UART to indicate speed increase or decrease */
const uint8_t speed_incr_char = '+';
const uint8_t speed_decr_char = '-';

/* Function prototypes */
int64_t alarm_handler(alarm_id_t id, __unused void *user_data);
void button_press_handler();
void uart_rx_handler();
void turn_single_led(int pin, bool on);
void init_hw_pins();
void init_uart();
void init_interrupts();
/* End of prototypes */

int main()
{
    init_hw_pins();
    init_uart();
    init_interrupts();
    DEBUG_UART("System initialized\n");

    uint32_t led_index = 0;
    /* set the first LED to "on" since the following logic sets the next LEDs to on/off accordingly, skipping the first one */
    turn_single_led(led_pins[led_index], true);
    sleep_ms(led_ms);

    while (true) {

        /* turn the current LED off, get ready for the next LED to be turned on */
        turn_single_led(led_pins[led_index], false);

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
        turn_single_led(led_pins[led_index], true);
        sleep_ms(led_ms);
    }

    return 0;
}

void init_hw_pins()
{
    /* Output enable for the LEDs */
    *(io_rw_32 *)(SIO_BASE + SIO_GPIO_OE_SET_OFFSET) = led_init_bitmask;
    /* Drive LED output to low */
    *(io_rw_32 *)(SIO_BASE + SIO_GPIO_OUT_OFFSET) = 0;

    for (int pin = 0; pin < (count_of(led_pins)); pin++)
    {
        /* Set IO function to SIO for all output pins */
        hw_write_masked((io_rw_32 *)(PADS_BANK0_GPIO + GPIO_REG_SIZE * led_pins[pin]),
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
        );
        
        /* Note the *2 operation which is due to the offset between the registers of 8 bytes in total */
        *(io_rw_32 *)(IO_BANK0_GPIO + 2 * GPIO_REG_SIZE * led_pins[pin]) = GPIO_FUNC_SIO;
    }

    /* Input enable for the button */
    *(io_rw_32 *)(SIO_BASE + SIO_GPIO_OE_CLR_OFFSET) = BIT(BUTTON_PIN);
    /* Pull-up resistor enable for the button, as well as input enable, final register state - ...7:0 = [...01001000] */
    hw_write_masked((io_rw_32 *)(PADS_BANK0_GPIO + GPIO_REG_SIZE * BUTTON_PIN),
            ((1u << PADS_BANK0_GPIO0_PUE_LSB) | 0u << PADS_BANK0_GPIO0_PDE_LSB) | PADS_BANK0_GPIO0_IE_BITS,
            PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_PDE_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS); 

    *(io_rw_32 *)(IO_BANK0_GPIO + 2 * GPIO_REG_SIZE * BUTTON_PIN) = GPIO_FUNC_SIO;

    /* Initialise UART pins */
    *(io_rw_32 *)(IO_BANK0_GPIO + 2 * GPIO_REG_SIZE * UART_TX_PIN) = GPIO_FUNC_UART;
    *(io_rw_32 *)(IO_BANK0_GPIO + 2 * GPIO_REG_SIZE * UART_RX_PIN) = GPIO_FUNC_UART;
}

void init_uart()
{
    /* Basic UART inits */
    uart_init(UART_ID, BAUD_RATE);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    uart_set_fifo_enabled(UART_ID, true);
}

void init_interrupts()
{
    /* Enable button interrupt on GPIO pin 16 */
    hw_set_bits((io_rw_32 *)(IO_BANK0_BASE + IO_BANK0_PROC0_INTE2_OFFSET), IO_BANK0_PROC0_INTE2_GPIO16_EDGE_LOW_BITS);
    irq_set_exclusive_handler(IO_IRQ_BANK0, button_press_handler);
    irq_set_enabled(IO_IRQ_BANK0, true);

    /* Enable UART interrupt */
    irq_set_exclusive_handler(UART0_IRQ, uart_rx_handler);
    irq_set_enabled(UART0_IRQ, true);

    /* set this to RX only */
    uart_set_irq_enables(UART_ID, true, false);
}

/* GPIO config / drive via registers instead of helper functions */
void turn_single_led(int pin, bool on)
{
    if (on)
    {
        *(io_rw_32 *)(SIO_BASE + SIO_GPIO_OUT_SET_OFFSET) = BIT(pin);
    }
    else
    {
        *(io_rw_32 *)(SIO_BASE + SIO_GPIO_OUT_CLR_OFFSET) = BIT(pin);
    }
}

/* Interrupt handlers */
int64_t alarm_handler(alarm_id_t id, __unused void *user_data)
{
    irq_set_enabled(IO_IRQ_BANK0, true);
    return 0;
}

void button_press_handler()
{
    hw_clear_bits((io_rw_32 *)(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET), IO_BANK0_INTR2_GPIO16_EDGE_LOW_BITS);
    irq_set_enabled(IO_IRQ_BANK0, false);

    direction_forward = !direction_forward;
    /* Disable the button pin interrupt and wait for the alarm to set it up again after a certain timeout defined in BUTTON_IRQ_TIMEOUT_MS.
    This shall serve (in addition to hardware button debouncing) as a way to change the light direction during the sleep_ms phase for the LEDs */
    add_alarm_in_ms(BUTTON_IRQ_TIMEOUT_MS, alarm_handler, NULL, false);
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
                DEBUG_UART("Speed increased\n");
            }
            else
            {
                led_ms = LED_MIN_MS;
                DEBUG_UART("Min LED time reached\n");
            }
        }
        else if (ch == speed_decr_char)
        {
            if ((led_ms + LED_CHANGE_STEP_MS) < LED_MAX_MS)
            {
                led_ms += LED_CHANGE_STEP_MS;
                DEBUG_UART("Speed decreased\n");
            }
            else
            {
                led_ms = LED_MAX_MS;
                DEBUG_UART("Max LED time reached\n");
            }
        }
        else
        {
            /* Invalid character, no change*/
            DEBUG_UART("Invalid character\n");
            continue;
        }
    }
}