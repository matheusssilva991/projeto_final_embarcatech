#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"

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
#define VRX_PIN 27
#define VRY_PIN 26
#define SW_PIN 22
#define ADC_MAX_VALUE 4096
#define MAX_TEMP 60

void init_led(uint8_t led_pin);
void init_btn(uint8_t btn_pin);
void init_leds();
void init_btns();
void init_i2c();
void init_display(ssd1306_t *ssd);
void init_joystick();
void read_joystick_xy_values(uint16_t *x_value, uint16_t *y_value);
void process_joystick_xy_values(uint16_t x_value_raw, uint16_t y_value_raw, float *x_value, float *y_value);

int main()
{
    // Inicializa o display OLED
    ssd1306_t ssd; // Inicializa a estrutura do display
    uint16_t vrx_value_raw;
    uint16_t vry_value_raw;
    float temperature; // Valor de X após processamento
    float humidity; // Valor de Y após processamento

    stdio_init_all();

    init_leds();
    init_btns();
    init_i2c();
    init_display(&ssd);
    ws2812b_init(LED_MATRIX_PIN); // Inicializa a matriz de LEDs
    adc_init();
    init_joystick();

    while (true) {
        read_joystick_xy_values(&vrx_value_raw, &vry_value_raw);
        process_joystick_xy_values(vrx_value_raw, vry_value_raw, &humidity, &temperature);

        // Imprime os valores lidos na comunicação serial.
        printf("VRX: %u, VRY: %u\n", vrx_value_raw, vry_value_raw);
        printf("TEMPERATURA: %1.f°, HUMIDADE: %1.f%%\n", temperature, humidity);

        ssd1306_draw_string(&ssd, "Quarto 1", 30, 4);
        ssd1306_draw_char(&ssd, 0xB0, 30, 30);
        ssd1306_draw_char(&ssd, '%', 36, 30);
        ssd1306_draw_char(&ssd, ':', 44, 30);
        ssd1306_draw_string(&ssd, "CAM: OFF", 30, 55);
        ssd1306_send_data(&ssd); // Atualiza o display

        sleep_ms(500);
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

// Inicializa o joystick
void init_joystick()
{
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);

    init_btn(SW_PIN);
}

// Realiza a amostragem e atualiza os valores X e Y do joystick
void read_joystick_xy_values(uint16_t *x_value, uint16_t *y_value)
{
    adc_select_input(1); // Seleciona o ADC para eixo X. O pino 26 como entrada analógica
    *x_value = adc_read();
    sleep_us(20);

    adc_select_input(0); // Seleciona o ADC para eixo Y. O pino 27 como entrada analógica
    *y_value = adc_read();
    sleep_us(20);
}

// Processa os valores de X e Y para a posição correto no display
void process_joystick_xy_values(uint16_t x_value_raw, uint16_t y_value_raw, float *x_value, float *y_value)
{
    *x_value = 100 * (float) x_value_raw / ADC_MAX_VALUE;
    *y_value = MAX_TEMP * (float)y_value_raw / ADC_MAX_VALUE;
}
