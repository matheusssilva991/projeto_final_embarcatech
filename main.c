#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

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
#define BUZZER_A_PIN 21
#define BUZZER_B_PIN 10
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
void pwm_init_buzzer(uint pin);
void play_tone(uint pin, uint frequency);
void read_joystick_xy_values(uint16_t *x_value, uint16_t *y_value);
void process_joystick_xy_values(uint16_t x_value_raw, uint16_t y_value_raw, float *x_value, float *y_value);
void draw_temperature_level(float temperature);
void gpio_irq_handler(uint gpio, uint32_t events);
int64_t buzzer_a_alarm_callback(alarm_id_t id, void *user_data);
int64_t buzzer_b_alarm_callback(alarm_id_t id, void *user_data);

room_t rooms[NUM_ROOM];
static volatile int room_id = 0;
static volatile int64_t last_valid_press_time_btn_a = 0;
static volatile int64_t last_valid_press_time_btn_b = 0;
static volatile int64_t last_valid_press_time_sw = 0;
static volatile bool full_recording = false;
static volatile bool buzzer_a_playing = false;
static volatile bool buzzer_b_playing = false;

int main()
{
    // Inicializa o display OLED
    ssd1306_t ssd; // Inicializa a estrutura do display
    uint16_t vrx_value_raw;
    uint16_t vry_value_raw;
    char temperature_text[20];
    char humidity_text[20];
    char cam_text[20];

    stdio_init_all();

    init_leds();
    init_btns();
    init_i2c();
    init_display(&ssd);
    ws2812b_init(LED_MATRIX_PIN); // Inicializa a matriz de LEDs
    adc_init();
    init_joystick();
    pwm_init_buzzer(10);
    pwm_init_buzzer(21);

    strcpy(rooms[0].name, "Sala");
    strcpy(rooms[1].name, "Quarto");
    strcpy(rooms[2].name, "Cozinha");

    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(SW_PIN, GPIO_IRQ_EDGE_FALL, true);

    while (true)
    {
        read_joystick_xy_values(&vrx_value_raw, &vry_value_raw);
        process_joystick_xy_values(vrx_value_raw, vry_value_raw, &rooms[room_id].humidity,
                                   &rooms[room_id].temperature);

        rooms[room_id].cam_on = rooms[room_id].temperature > 37 ? true : false;

        // Formata a string e armazena em temperature_text
        snprintf(temperature_text, sizeof(temperature_text), "Temp:%3.0f°", rooms[room_id].temperature);

        // Formata a string e armazena em humidity_text
        snprintf(humidity_text, sizeof(humidity_text), "Hum:%3.0f%%", rooms[room_id].humidity);

        // Formata a string e armazena em cam_text
        if (full_recording) { // Ativa modo gravação total
            snprintf(cam_text, sizeof(cam_text), "Cam: Full On");
        }
        else if (rooms[room_id].cam_on) // Ativa a gravação caso a temperatura esteja alta
        {
            snprintf(cam_text, sizeof(cam_text), "Cam:On");
        }
        else // Desliga a gravação para temperaturas amenas
        {
            snprintf(cam_text, sizeof(cam_text), "Cam:Off");
        }

        // Desenha as informações no display SSD1306
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, rooms[room_id].name, 30, 4);
        ssd1306_draw_string(&ssd, temperature_text, 30, 26);
        ssd1306_draw_string(&ssd, humidity_text, 30, 37);
        ssd1306_draw_string(&ssd, cam_text, 30, 55);
        ssd1306_send_data(&ssd); // Atualiza o display

        // Mostra o nivel da temperatura na matriz de LED
        draw_temperature_level(rooms[room_id].temperature);

        if (rooms[room_id].temperature < 7 && !buzzer_a_playing) {
            buzzer_a_playing = true;
            play_tone(BUZZER_A_PIN, 300);
            add_alarm_in_ms(5000, buzzer_a_alarm_callback, NULL, false);
        } else if (rooms[room_id].temperature > 44 && !buzzer_b_playing) {
            buzzer_b_playing = true;
            play_tone(BUZZER_B_PIN, 415);
            add_alarm_in_ms(5000, buzzer_b_alarm_callback, NULL, false);
        }

        // Imprime os valores lidos na comunicação serial.
        printf("VRX: %u, VRY: %u\n", vrx_value_raw, vry_value_raw);
        printf("TEMPERATURA: %1.f°, HUMIDADE: %1.f%%\n", rooms[room_id].temperature, rooms[room_id].humidity);

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

// Inicializa o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint pin, uint frequency) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2); // 50% de duty cycle
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

// Desenha o nível de temperatura na matriz de LED
void draw_temperature_level(float temperature)
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

// Função de callback dos botões
void gpio_irq_handler(uint gpio, uint32_t events) {
    int64_t current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BTN_A_PIN && current_time - last_valid_press_time_btn_a > 250) {
        room_id = room_id - 1 >= 0 ? room_id - 1: 0;
        last_valid_press_time_btn_a = to_ms_since_boot(get_absolute_time());

    } else if (gpio == BTN_B_PIN && current_time - last_valid_press_time_btn_b > 250) {
        room_id = room_id + 1 <= NUM_ROOM - 1 ? room_id + 1: NUM_ROOM - 1;
        last_valid_press_time_btn_b = to_ms_since_boot(get_absolute_time());

    } else if (gpio == SW_PIN && current_time - last_valid_press_time_sw > 250) {
        full_recording = !full_recording;
        last_valid_press_time_sw = to_ms_since_boot(get_absolute_time());
    }
}

int64_t buzzer_a_alarm_callback(alarm_id_t id, void *user_data) {
    buzzer_a_playing = false;
    pwm_set_gpio_level(BUZZER_A_PIN, 0); // Desliga o som após a duração
}

int64_t buzzer_b_alarm_callback(alarm_id_t id, void *user_data) {
    buzzer_b_playing = false;
    pwm_set_gpio_level(BUZZER_B_PIN, 0); // Desliga o som após a duração
}
