#include "libs/pico-ssd1306/ssd1306.h"
#include "string.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display
#define I2C_SDA 14          // Pino SDA
#define I2C_SCL 15          // Pino SCL

ssd1306_t display;

/**
 * @brief Inicializa o display OLED ssd1306
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
 * @brief Escreve um determinado texto na tela
 * @param[in] text Texto a ser exibido
 * @param[in] posX Posição horizontal de exibição do texto
 * @param[in] posY Posição vertical da exibição do texto
 * @return void
 */
void oled_write(char *text, uint32_t posX, uint32_t posY){
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, posX, posY, 1, text);
    ssd1306_show(&display);
}


/**
 * @brief Escreve um determinado texto na tela, mas sem limpar ela. Serve para adicionar múltiplos textos na tela
 * @param[in] text Texto a ser exibido
 * @param[in] posX Posição horizontal de exibição do texto
 * @param[in] posY Posição vertical da exibição do texto
 * @return void
 */
void oled_write_no_clear(char *text, uint32_t posX,uint32_t posY){
    ssd1306_draw_string(&display, posX, posY, 1, text);
    ssd1306_show(&display);
}

/**
 * @brief Atualiza a seleção no menu principal
 */
void draw_main_menu_selection(int select_max) {
    if (select_max) {
        oled_write_no_clear("> Ajustar Temp. MAX", 0, 16);
        oled_write_no_clear("  Ajustar Temp. MIN", 0, 28);
    } else {
        oled_write_no_clear("  Ajustar Temp. MAX", 0, 16);
        oled_write_no_clear("> Ajustar Temp. MIN", 0, 28);
    }
}

/**
 * @brief Desenha o menu principal no display OLED
 */
void draw_main_menu(int *temp_min, int *temp_max, int menu_select) {
    oled_write("=== MENU PRINCIPAL ===", 0, 0);
    draw_main_menu_selection(menu_select);
    oled_write_no_clear("Atual MAX: ", 0, 44);
    char temp_buffer[10];
    sprintf(temp_buffer, "%d C", *temp_max);
    oled_write_no_clear(temp_buffer, 80, 44);
    oled_write_no_clear("Atual MIN: ", 0, 56);
    sprintf(temp_buffer, "%d C", *temp_min);
    oled_write_no_clear(temp_buffer, 80, 56);
}


/**
 * @brief Tela de ajuste de temperatura máxima
 */
void draw_set_temp_max(int temp_max_setting) {
    oled_write("== AJUSTE TEMP. MAX ==", 0, 0);
    oled_write_no_clear("Use joystick p/ ajustar", 0, 16);
    char temp_buffer[20];
    sprintf(temp_buffer, "Valor atual: %d C", temp_max_setting);
    oled_write_no_clear(temp_buffer, 0, 28);
    oled_write_no_clear("ENTER para confirmar", 0, 44);
    oled_write_no_clear("BACK para cancelar", 0, 56);
}

/**
 * @brief Tela de ajuste de temperatura mínima
 */
void draw_set_temp_min(int temp_min_setting) {
    oled_write("== AJUSTE TEMP. MIN ==", 0, 0);
    oled_write_no_clear("Use joystick p/ ajustar", 0, 16);
    char temp_buffer[20];
    sprintf(temp_buffer, "Valor atual: %d C", temp_min_setting);
    oled_write_no_clear(temp_buffer, 0, 28);
    oled_write_no_clear("ENTER para confirmar", 0, 44);
    oled_write_no_clear("BACK para cancelar", 0, 56);
}