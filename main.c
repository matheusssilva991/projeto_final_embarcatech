#include <stdio.h>
#include <string.h>
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
#define MAX_TEMP 62
#define NUM_ROOM 3

typedef struct room
{
    char name[10];
    float temperature;
    float humidity;
    bool cam_on;
} room_t;

void init_led(uint8_t led_pin);
void init_btn(uint8_t btn_pin);
void init_leds();
void init_btns();
void init_i2c();
void init_display(ssd1306_t *ssd);
void init_joystick();
void read_joystick_xy_values(uint16_t *x_value, uint16_t *y_value);
void process_joystick_xy_values(uint16_t x_value_raw, uint16_t y_value_raw, float *x_value, float *y_value);
void blink_temperature(float temperature);
void gpio_irq_handler(uint gpio, uint32_t events);

room_t rooms[NUM_ROOM];
static volatile int room_id = 0;
static volatile int64_t btn_a_time = 0;
static volatile int64_t btn_b_time = 0;

int main()
{
    // Inicializa o display OLED
    ssd1306_t ssd; // Inicializa a estrutura do display
    uint16_t vrx_value_raw;
    uint16_t vry_value_raw;
    char temperature_text[20];
    char humidity_text[20];
    char cam_text[10];

    stdio_init_all();

    init_leds();
    init_btns();
    init_i2c();
    init_display(&ssd);
    ws2812b_init(LED_MATRIX_PIN); // Inicializa a matriz de LEDs
    adc_init();
    init_joystick();

    strcpy(rooms[0].name, "Sala");
    strcpy(rooms[1].name, "Quarto");
    strcpy(rooms[2].name, "Cozinha");

    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true);

    while (true)
    {
        read_joystick_xy_values(&vrx_value_raw, &vry_value_raw);
        process_joystick_xy_values(vrx_value_raw, vry_value_raw, &rooms[room_id].humidity,
                                   &rooms[room_id].temperature);
        rooms[room_id].cam_on = rooms[room_id].temperature > 37 ? true : false;

        // Imprime os valores lidos na comunicação serial.
        printf("VRX: %u, VRY: %u\n", vrx_value_raw, vry_value_raw);
        printf("TEMPERATURA: %1.f°, HUMIDADE: %1.f%%\n", rooms[room_id].temperature, rooms[room_id].humidity);

        // Formata a string e armazena em temperature_text
        snprintf(temperature_text, sizeof(temperature_text), "Temp:%3.0f°", rooms[room_id].temperature);

        // Formata a string e armazena em humidity_text
        snprintf(humidity_text, sizeof(humidity_text), "Hum:%3.0f%%", rooms[room_id].humidity);

        // Formata a string e armazena em cam_text
        if (rooms[room_id].cam_on)
        {
            snprintf(cam_text, sizeof(cam_text), "Cam:ON");
        }
        else
        {
            snprintf(cam_text, sizeof(cam_text), "Cam:OFF");
        }

        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, rooms[room_id].name, 30, 4);
        ssd1306_draw_string(&ssd, temperature_text, 30, 26);
        ssd1306_draw_string(&ssd, humidity_text, 30, 37);
        ssd1306_draw_string(&ssd, cam_text, 30, 55);
        ssd1306_send_data(&ssd); // Atualiza o display

        blink_temperature(rooms[room_id].temperature);

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
void init_btn(uint8_t btn_pin)
{
    gpio_init(btn_pin);
    gpio_set_dir(btn_pin, GPIO_IN);
    gpio_pull_up(btn_pin);
}

// Inicializa o LED RGB (11 - Azul, 12 - Verde, 13 - Vermelho)
void init_leds()
{
    init_led(BLUE_LED_PIN);
    init_led(GREEN_LED_PIN);
    init_led(RED_LED_PIN);
}

// Inicializa os botões A e B (5 - A, 6 - B)
void init_btns()
{
    init_btn(BTN_A_PIN);
    init_btn(BTN_B_PIN);
}

// Inicializa a comunicação I2C
void init_i2c()
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

// Inicializa o display OLED
void init_display(ssd1306_t *ssd)
{
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
    *x_value = 100 * (float)x_value_raw / ADC_MAX_VALUE;
    *y_value = MAX_TEMP * (float)y_value_raw / ADC_MAX_VALUE;
}

void blink_temperature(float temperature)
{
    ws2812b_clear();

    /* if (temperature >= 0) {
        for (int i=0; i < 5; i++) {
            ws2812b_set_led(i, 0, 0, 8);
        }
    }

    if (temperature >= 10) {
        for (int i=5; i < 10; i++) {
            ws2812b_set_led(i, 0, 0, 8);
        }
    }

    if (temperature >= 18) {
        for (int i=10; i < 15; i++) {
            ws2812b_set_led(i, 0, 8, 0);
        }
    }

    if (temperature >= 33) {
        for (int i=15; i < 20; i++) {
            ws2812b_set_led(i, 8, 0, 0);
        }
    }

    if (temperature >= 42) {
        for (int i=20; i < 25; i++) {
            ws2812b_set_led(i, 8, 0, 0);
        }
    } */

    ws2812b_write();
}

void gpio_irq_handler(uint gpio, uint32_t events) {
    int64_t current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BTN_A_PIN && current_time - btn_a_time > 300) {
        room_id = room_id - 1 >= 0 ? room_id - 1: 0;
    } else if (gpio == BTN_B_PIN && current_time - btn_b_time > 300) {
        room_id = room_id + 1 <= NUM_ROOM - 1 ? room_id + 1: NUM_ROOM - 1;
    }
}
