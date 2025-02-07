#include "pico/time.h"
#include "hardware/address_mapped.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/regs/sio.h"

#define BIT(n)  (1u<<(n))
#define LED_MS 250

#define BUTTON_PIN 16

#define PADS_BANK0_GPIO (PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET)
#define IO_BANK0_GPIO (IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET)
#define GPIO_FUNC_SIO 5
#define GPIO_REG_SIZE sizeof(io_rw_32)

uint8_t led_pins[] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
uint32_t led_init_bitmask = BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7) | BIT(8) | BIT(9) | BIT(10);
char direction_forward = true;

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

bool button_read_high()
{
    return *(io_rw_32 *)(SIO_BASE + SIO_GPIO_IN_OFFSET) & BIT(BUTTON_PIN);
}

void init_hw_pins()
{
    // output enable for the LEDs
    *(io_rw_32 *)(SIO_BASE + SIO_GPIO_OE_SET_OFFSET) = led_init_bitmask;
    // drive LED output to low
    *(io_rw_32 *)(SIO_BASE + SIO_GPIO_OUT_OFFSET) = 0;

    for (int pin = 0; pin < (count_of(led_pins)); pin++)
    {
        // set IO function to SIO for all output pins
        hw_write_masked((io_rw_32 *)(PADS_BANK0_GPIO + GPIO_REG_SIZE * led_pins[pin]),
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
        );
        
        *(io_rw_32 *)(IO_BANK0_GPIO + 2 * GPIO_REG_SIZE * led_pins[pin]) = GPIO_FUNC_SIO;
    }

    // input enable for the button
    *(io_rw_32 *)(SIO_BASE + SIO_GPIO_OE_CLR_OFFSET) = BIT(BUTTON_PIN);
    // pull-up resistor enable for the button, as well as input enable
    // final register state - ...7:0 = [...01001000]
    hw_write_masked((io_rw_32 *)(PADS_BANK0_GPIO + GPIO_REG_SIZE * BUTTON_PIN),
            ((1u << PADS_BANK0_GPIO0_PUE_LSB) | 0u << PADS_BANK0_GPIO0_PDE_LSB) | PADS_BANK0_GPIO0_IE_BITS,
            PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_PDE_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS); 

    *(io_rw_32 *)(IO_BANK0_GPIO + 2 * GPIO_REG_SIZE * BUTTON_PIN) = GPIO_FUNC_SIO;
}

int main()
{
    init_hw_pins();
    
    uint32_t led_index = 0;
    // set the first LED to "on" since the following logic sets the next LEDs to on/off accordingly, skipping the first one
    turn_single_led(led_pins[led_index], true);

    sleep_ms(LED_MS);

    while (true) {

        // toggle direction if button is pressed
        if (!button_read_high())
        {
            direction_forward = !direction_forward;
        }

        // turn the current LED off, get ready for the next LED to be turned on
        turn_single_led(led_pins[led_index], false);

        if (direction_forward == true)
        {
            if (led_index == (count_of(led_pins) - 1))
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
            if (led_index == 0)
            {
                led_index = count_of(led_pins) - 1;
            }
            else
            {
                led_index--;
            }
        }

        turn_single_led(led_pins[led_index], true);
        sleep_ms(LED_MS);
    }

    return 0;
}