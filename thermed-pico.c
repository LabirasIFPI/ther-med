#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h" // Biblioteca para PWM
#include "hardware/timer.h" 
#include "utils/led_matrix_funcs.h" // Funcoes para controlar a matriz de LEDS
#include "utils/display_funcs.h" // Funcoes para controlar o display OLED

#define DHT_PIN 8         // Definição do GPIO onde o DHT22 está conectado
#define BUZZER_PIN 21     // Definição do GPIO onde o buzzer passivo está conectado

#define TEMP_LIMIT_MIN 0  // Limite mínimo de temperatura
#define TEMP_LIMIT_MAX 40 // Limite máximo de temperatura



void buzzer_init() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);  // Define o pino como saída PWM
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice_num, 50000);  // Define a frequência
    pwm_set_gpio_level(BUZZER_PIN, 10000); // Define o duty cycle
    pwm_set_enabled(slice_num, 0); // Começa desativado
}

void buzzer_on() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, 1); // Ativa o som
}

void buzzer_off() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, 0); // Desativa o som
}

/**
 * Realiza a leitura da temperatura no sensor DHT22
 */
void dht22_read(int *temperature) {
    uint32_t data = 0;
    uint8_t bits[5] = {0};

    // Configura o pino do sensor como saída e manda um pulso baixo
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(18);
    gpio_put(DHT_PIN, 1);
    sleep_us(40);

    // Muda para entrada para receber dados
    gpio_set_dir(DHT_PIN, GPIO_IN);

    // Espera o sensor enviar algum dado
    while (gpio_get(DHT_PIN) == 1);
    while (gpio_get(DHT_PIN) == 0);
    while (gpio_get(DHT_PIN) == 1);

    // Lê 40 bits (5 bytes) de dados
    for (int i = 0; i < 40; i++) {
        while (gpio_get(DHT_PIN) == 0);  // Aguarda o início do bit
        sleep_us(28);  // Espera um pouco para diferenciar 0 de 1

        if (gpio_get(DHT_PIN) == 1) {
            bits[i / 8] |= (1 << (7 - (i % 8)));
        };

        while (gpio_get(DHT_PIN) == 1);
    }

    // Verifica se os dados são válidos
    if ((bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) {
        *temperature = bits[2];
    } else {
        *temperature = -1;  // Erro na leitura
    }
}

void check_temperature(int *temp) {
    char temperature_buffer[30];

    if (*temp != -1) {
        printf("Temperatura: %d°C\n", *temp);
        
        if (*temp >= TEMP_LIMIT_MAX) { // Se a temperatura estiver muito alta, ativa o buzzer
            buzzer_on();
        } else {
            buzzer_off();
        }

        led_matrix_colorize(GRB_GREEN);
        sprintf(temperature_buffer, "Temperatura: %d graus", *temp);
        oled_write(temperature_buffer, 0, 32);

    } else {
        printf("Erro ao ler o DHT22\n");
        led_matrix_colorize(GRB_YELLOW);
        oled_write("Erro ao ler sensor!", 0, 24);
        oled_write_no_clear("Verifique as conexoes!", 0, 36);
        buzzer_off(); // Desliga o buzzer se houver erro
    }
}

/**
 * Realiza a inicialização dos sensores e atuadores do projeto
 */
void setup() {
    stdio_init_all();
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    buzzer_init();

    printf("Inicializando DHT22...\n");
    gpio_init(DHT_PIN);
    
    printf("Inicializando matriz de LEDs...\n");
    led_matrix_init();

    printf("Inicializando display OLED...\n");
    while (oled_display_init()){
        printf("Falha ao inicializar display!\n");
        printf("Tentando novamente...\n");
    }
    
    oled_write("Display inicializado!", 0, 32);
}

int main() {
    setup();
    int temperature;

    while (true) {
        dht22_read(&temperature);
        check_temperature(&temperature);
        sleep_ms(1000);  // Aguarda 1 segundo antes da próxima leitura
    }

    return 0;
}
