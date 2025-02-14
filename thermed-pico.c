#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "ws2812b_animation.h" // Matriz de LEDs

#define LED_MATRIX_PIN 7 // Definição do GPIO da matriz de LEDs RGB
#define DHT_PIN 8  // Definição do GPIO onde o DHT22 está conectado

#define TEMP_LIMIT_MIN
#define TEMP_LIMIT_MAX

/**
 * Realiza a leitura do sensor de temperatura e umidade
 */
void dht11_read(int *temperature, int *humidity) {
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
        if (gpio_get(DHT_PIN) == 1) bits[i / 8] |= (1 << (7 - (i % 8)));
        while (gpio_get(DHT_PIN) == 1);

    }

    // Verifica se os dados são válidos
    if ((bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) {
        *humidity = bits[0];
        *temperature = bits[2];

    } else {
        *temperature = -1;  // Erro na leitura
        *humidity = -1;
    }
}

/**
 * Realiza a inicialização de todos os sensores e atuadores do projeto
 */
void setup(){
    stdio_init_all();

    printf("Inicializando DHT11...\n");
    gpio_init(DHT_PIN);
    
    printf("Inicializando matriz de LEDs");
    ws2812b_init(pio0, LED_MATRIX_PIN, 25);
    ws2812b_set_global_dimming(7);
    ws2812b_fill_all(GRB_SPRING);
    ws2812b_render();
}

int main() {
    setup();

    while (true) {
        int temperature, humidity;
        dht11_read(&temperature, &humidity);

        if (temperature != -1 && humidity != -1) {
            printf("Temperatura: %d°C, Umidade: %d%%\n", temperature, humidity);
        } else {
            printf("Erro ao ler o DHT11\n");
        }

        sleep_ms(2000);  // Aguarda 2 segundos antes da próxima leitura
    }

    return 0;
}