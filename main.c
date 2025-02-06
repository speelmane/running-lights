#include "pico/stdlib.h"

#define BIT(n)  (1u<<(n))
#define LED_MS 250

#define BUTTON_PIN 16
// optimize
uint8_t led_pins[] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
uint32_t led_init_bitmask = BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7) | BIT(8) | BIT(9) | BIT(10);
char direction_forward = true;

void init_pins()
{
    gpio_init_mask(led_init_bitmask);
    gpio_set_dir_out_masked(led_init_bitmask);

    // button initialisation with an internal pull-up resistor, the direction in gpio_init by default is GPIO_IN
    gpio_init(BUTTON_PIN);
    gpio_pull_up(BUTTON_PIN);
}

int main()
{
    stdio_init_all();
    init_pins();
    
    uint32_t led_index = 0;
    // set the first LED to "on" since the following logic sets the next LEDs to on/off accordingly, skipping the first one
    gpio_put(led_pins[led_index], true);
    sleep_ms(LED_MS);

    while (true) {

        // toggle direction if button is pressed
        if (!gpio_get(BUTTON_PIN))
        {
            direction_forward = !direction_forward;
        }

        // turn the current LED off, get ready for the next LED to be turned on
        gpio_put(led_pins[led_index], false);

        if (direction_forward == true)
        {
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
            if (led_index == 0)
            {
                led_index = sizeof(led_pins) - 1;
            }
            else
            {
                led_index--;
            }
        }

        gpio_put(led_pins[led_index], true);
        sleep_ms(LED_MS);
    }

    return 0;
}