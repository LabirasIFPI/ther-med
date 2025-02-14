#include "ws2812b_animation.h"
#define LED_MATRIX_PIN 7  // Definição do GPIO da matriz de LEDs RGB

/**
 * Inicializa a matriz de LEDs, colorizando-a inicialmente para fins de teste
 */
void led_matrix_init(){
    ws2812b_init(pio0, LED_MATRIX_PIN, 25);
    ws2812b_set_global_dimming(7);
    ws2812b_fill_all(GRB_SPRING);
    ws2812b_render();
}

/**
 * Coloriza a matriz de LEDs com uma determinada cor
 */
void led_matrix_colorize(uGRB32_t color){
    ws2812b_fill_all(color);
    ws2812b_render();
}