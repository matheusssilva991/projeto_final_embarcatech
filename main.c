#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"

#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/ws2812b.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define I2C_ADDRESS 0x3C
#define LED_MATRIX_PIN 7
#define GREEN_LED_PIN 11
#define BLUE_LED_PIN 12
#define RED_LED_PIN 13
#define BTN_A_PIN 5
#define BTN_B_PIN 6

void init_led(uint8_t led_pin);
void init_btn(uint8_t btn_pin);
void init_leds();
void init_btns();
void init_i2c();
void init_display(ssd1306_t *ssd);


int main()
{
    // Inicializa o display OLED
    ssd1306_t ssd; // Inicializa a estrutura do display

    stdio_init_all();

    init_leds();
    init_btns();
    init_display(&ssd);
    init_i2c();
    ws2812b_init(LED_MATRIX_PIN); // Inicializa a matriz de LEDs


    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}

// Inicializa um led em um pino específico
void init_led(uint8_t led_pin)
{
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
}

// Inicializa um botão em um pino específico
void init_btn(uint8_t btn_pin) {
    gpio_init(btn_pin);
    gpio_set_dir(btn_pin, GPIO_IN);
    gpio_pull_up(btn_pin);
}

// Inicializa o LED RGB (11 - Azul, 12 - Verde, 13 - Vermelho)
void init_leds() {
    init_led(BLUE_LED_PIN);
    init_led(GREEN_LED_PIN);
    init_led(RED_LED_PIN);
}

// Inicializa os botões A e B (5 - A, 6 - B)
void init_btns() {
    init_btn(BTN_A_PIN);
    init_btn(BTN_B_PIN);
}

// Inicializa a comunicação I2C
void init_i2c() {
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

// Inicializa o display OLED
void init_display(ssd1306_t *ssd) {
    ssd1306_init(ssd, WIDTH, HEIGHT, false, I2C_ADDRESS, I2C_PORT);
    ssd1306_config(ssd);
    ssd1306_send_data(ssd);

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(ssd, false);
    ssd1306_send_data(ssd);
}
