#include "libs/pico-ssd1306/ssd1306.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display
#define I2C_SDA 14          // Pino SDA
#define I2C_SCL 15          // Pino SCL

ssd1306_t display;

/**
 * Inicializa o display OLED ssd1306
 */
int oled_display_init(){
    // Inicializa I2C no canal 1
    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) { 
        return 1; // Falha ao inicializar display
    }

    return 0;
}

/**
 * Escreve um determinado texto na tela
 * @param[in] text Texto a ser exibido
 * @param[in] posX Posição horizontal de exibição do texto
 * @param[in] posY Posição vertical da exibição do texto
 * @return void
 */
void oled_write(char *text, uint32_t posX,uint32_t posY){
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, posX, posY, 1, text);
    ssd1306_show(&display);
}


/**
 * Escreve um determinado texto na tela, mas sem limpar ela. Serve para adicionar múltiplos textos na tela
 * @param[in] text Texto a ser exibido
 * @param[in] posX Posição horizontal de exibição do texto
 * @param[in] posY Posição vertical da exibição do texto
 * @return void
 */
void oled_write_no_clear(char *text, uint32_t posX,uint32_t posY){
    ssd1306_draw_string(&display, posX, posY, 1, text);
    ssd1306_show(&display);
}