#include "pico/stdlib.h"

#define BIT(n)  (1u<<(n))
#define LED_MS 500

// optimize
uint8_t led_pins[] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
uint32_t gpio_bitmask = BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7) | BIT(8) | BIT(9) | BIT(10);

void led_init()
{
    gpio_init_mask(gpio_bitmask);
    gpio_set_dir_out_masked(gpio_bitmask);
}


int main() {
    stdio_init_all();
    led_init();
    

    while (true) {
        for (int i = 0; i < sizeof(led_pins); i++)
        {
            gpio_put(led_pins[i], true);
            if (i == 0)
            {
                gpio_put(led_pins[sizeof(led_pins) - 1], false);
            }
            else
            {
                gpio_put(led_pins[i - 1], false);
            }
            sleep_ms(LED_MS);
        }

        // gpio_put_masked(gpio_bitmask, false);
    }
}