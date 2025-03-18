#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h" // Temporizador para o alarme
#include "hardware/adc.h"   // API do conversor ADC para uso do joystick
#include "pico/time.h"
#include "pico/unique_id.h" // API para identificação unica do raspberry pi pico w

#include "utils/alarm_funcs.h"
#include "utils/led_matrix_funcs.h"   // Funcoes para controlar a matriz de LEDS
#include "utils/display_funcs.h"      // Funcoes para controlar o display OLED
#include "utils/connection_manager.h" // Funcoes para gerenciar o envio de alertas via wi-fi

#define BUTTON_ENTER 5
#define BUTTON_BACK 6
#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define DHT_PIN 8                   // Definição do GPIO onde o DHT22 está conectado
#define ALARM_PULSE_INTERVAL 500000 // Intervalo de pulsação do buzzer em microssegundos

/**
 * @brief Enumeração de estados do sistema
 */
typedef enum SystemState {
    /* O sistema está em estado de monitoramento dos sensores */
    STATE_MONITORING,

    /*O sistema se encontra no menu principal */
    STATE_MENU_MAIN,

    /* Página do menu para alterar o limite máximo */
    STATE_MENU_SET_MAX,

    /* Página do menu para alterar o limite mínimo */
    STATE_MENU_SET_MIN
} SystemState;

// Variável global para armazenar o identificador único do dispositivo
char device_id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];

// Variáveis globais para o sistema do menu
SystemState current_state = STATE_MONITORING;
int temp_max = 32;
int temp_max_setting = 0;
int temp_min = -8;
int temp_min_setting = 0;
int selected_max = 1;
int button_enter_pressed = 0;
int button_back_pressed = 0;
int joystick_button_pressed = 0;
int joystick_x_value = 0;
int joystick_y_value = 0;

// Configurações de wi-fi e API
wifi_config_t wifi_config = {
    .ssid = "SEU_SSID",
    .senha = "SUA_SENHA",
    .api_host = "example.com",    // Host da API (placeholder)
    .api_port = 80,               // Porta da API
    .api_url = "/alarme"          // Endpoint da API
};

// Configurações para debounce do botão
#define DEBOUNCE_TIME 200
uint32_t *last_button_time = 0;
uint32_t *current_time = 0;

/**
 * @brief Configura o identificador único do dispositivo ao inicializar
 */
void setup_device_id(){
    pico_get_unique_board_id_string(device_id, 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1);

    printf("Device ID: %s\n", device_id);
}

/**
 * @brief Inicializa os pinos para os botões e joystick
 */
void buttons_joystick_init() {
    // Inicializa os pinos dos botões que vão ser usados
    gpio_init(BUTTON_ENTER);
    gpio_set_dir(BUTTON_ENTER, GPIO_IN);
    gpio_pull_up(BUTTON_ENTER);
    
    gpio_init(BUTTON_BACK);
    gpio_set_dir(BUTTON_BACK, GPIO_IN);
    gpio_pull_up(BUTTON_BACK);
    
    // Inicializar conversor adc para o joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
}

/**
 * @brief Lê os valores de cada botão, com debounce integrado
 */
void read_buttons(){
    *current_time = to_ms_since_boot(get_absolute_time());

    // Debounce
    if (*current_time - *last_button_time > DEBOUNCE_TIME){
        // Verificando estado de cada botão e atualizando-o
        if (!gpio_get(BUTTON_ENTER) && !button_enter_pressed){
            button_enter_pressed = 1;
            last_button_time = current_time;
        } else if (gpio_get(BUTTON_ENTER)){
            button_enter_pressed = 0;
        }

        if (!gpio_get(BUTTON_BACK) && !button_back_pressed){
            button_back_pressed = 1;
            last_button_time = current_time;
        } else if (gpio_get(BUTTON_BACK)){
            button_back_pressed = 0;
        }
    }

    // Leitura do joystick
    adc_select_input(0);
    joystick_y_value = adc_read();
    
    adc_select_input(1);
    joystick_x_value = adc_read();

    printf("Joy X: %d Joy Y: %d\n", joystick_x_value, joystick_y_value);

}

void process_menu(SystemState *current_state, int *temp_max, int *temp_min){
    // Detectando movimento do joystick
    int joystick_up = joystick_y_value > 3000;
    int joystick_down = joystick_y_value < 1000;

    switch (*current_state){
        case STATE_MONITORING:
            // Troca de contexto para o menu de calibração
            if (button_enter_pressed){
                *current_state = STATE_MENU_MAIN;
                temp_max_setting = *temp_max;
                temp_min_setting = *temp_min;
                button_enter_pressed = false; // resetar flag

                draw_main_menu(temp_min, temp_max, selected_max);
            }
            break;

        case STATE_MENU_MAIN:
            // Alternando entre opções do menu
            if (joystick_up || joystick_down){
                selected_max = !selected_max;
                draw_main_menu(temp_min, temp_max, selected_max);
                sleep_ms(200); // Delay para evitar mudancas muito rapidas
            }

            if (button_enter_pressed){
                *current_state = selected_max ? STATE_MENU_SET_MAX : STATE_MENU_SET_MIN;
                button_enter_pressed = 0;

                if (*current_state == STATE_MENU_SET_MAX){
                    draw_set_temp_max(*temp_max);

                } else {
                    draw_set_temp_min(*temp_min);
                }
            }

            if (button_back_pressed){
                // Voltar ao monitoramento
                *current_state = STATE_MONITORING;
                button_back_pressed = 0;
            }
            break;

        case STATE_MENU_SET_MAX:
            // Ajustar o limite máximo de temperatura
            if (joystick_up && temp_max_setting < 50) {  // Limite arbitrário de 50°C
                temp_max_setting++;
                draw_set_temp_max(temp_max_setting);
                sleep_ms(200);
            }
            
            if (joystick_down && temp_max_setting > *temp_min + 1) {
                temp_max_setting--;
                draw_set_temp_max(temp_max_setting);
                sleep_ms(200);
            }
            
            if (button_enter_pressed) {
                // Confirmar e salvar a configuração
                *temp_max = temp_max_setting;
                *current_state = STATE_MENU_MAIN;
                button_enter_pressed = 0;
                draw_main_menu(temp_min, temp_max, selected_max);
            }
            
            if (button_back_pressed) {
                // Cancelar e voltar sem salvar
                temp_max_setting = *temp_max;  // Restaurar valor original
                *current_state = STATE_MENU_MAIN;
                button_back_pressed = 0;
                draw_main_menu(temp_min, temp_max, selected_max);
            }
            break;

        case STATE_MENU_SET_MIN:

            // Ajustar o limite mínimo de temperatura
            if (joystick_up && temp_min_setting < temp_max_setting - 1) {
                temp_min_setting++;
                draw_set_temp_min(temp_min_setting);
                sleep_ms(200);
            }
            
            if (joystick_down && temp_min_setting > -20) {  // Limite arbitrário de -20°C
                temp_min_setting--;
                draw_set_temp_min(temp_min_setting);
                sleep_ms(200);
            }
            
            if (button_enter_pressed) {
                // Confirmar e salvar a configuração
                *temp_min = temp_min_setting;
                *current_state = STATE_MENU_MAIN;
                button_enter_pressed = 0;
                draw_main_menu(temp_min, temp_max, selected_max);
            }
            
            if (button_back_pressed) {
                // Cancelar e voltar sem salvar
                temp_min_setting = *temp_min;  // Restaurar valor original
                *current_state = STATE_MENU_MAIN;
                button_back_pressed = 0;
                draw_main_menu(temp_min, temp_max, selected_max);
            }
            break;
    }
}

/**
 * @brief Realiza a leitura da temperatura no sensor DHT22
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

/**
 * @brief Faz o controle de alarmes com base nos valores de temperatura observados.
 */
void check_temperature(int *temp) {
    char temperature_buffer[30];

    if (*temp != -1) {
        printf("Temperatura: %d°C\n", *temp);
        sprintf(temperature_buffer, "Temperatura: %d graus", *temp);
        oled_write(temperature_buffer, 0, 32);
        
        // Exibe também os limites configurados
        sprintf(temperature_buffer, "Limites: %d a %d graus", temp_min, temp_max);
        oled_write_no_clear(temperature_buffer, 0, 12);
        if (*temp >= temp_max || *temp <= temp_min) { // Se a temperatura estiver muito alta ou baixa
            if (!alarm_active){
                /*TODO: CHAMAR A FUNCAO DE ENVIAR ALERTA PARA A API, VERIFICANDO A PROCEDENCIA*/

                // Alarmes são disparados
                buzzer_on();
                alarm_active = true;
                add_repeating_timer_us(ALARM_PULSE_INTERVAL, alarm_toggle_callback,NULL,&alarm_timer);
            }
            
            // Mostra qual limite foi violado, o superior ou o inferior
            if (*temp >= temp_max) {
                oled_write_no_clear("Temperatura ALTA!", 0, 36);
            } else {
                oled_write_no_clear("Temperatura BAIXA!", 0, 36);
            }

        } else {
            buzzer_off();
            alarm_active = false;
            cancel_repeating_timer(&alarm_timer);
            led_matrix_colorize(GRB_GREEN);
        }


    } else {
        // Em caso de erro no sensor
        printf("Erro ao ler o DHT22\n");
        led_matrix_colorize(GRB_YELLOW);
        oled_write("Erro ao ler sensor!", 0, 24);
        oled_write_no_clear("Verifique as conexoes!", 0, 36);
        
        buzzer_off(); // Desliga os alarmes se houver erro
        alarm_active = false;
        cancel_repeating_timer(&alarm_timer);
    }
}

/**
 * @brief Realiza a inicialização dos sensores e atuadores do projeto
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

    printf("Inicializando botões e joystick...\n");
    buttons_joystick_init();
    
    oled_write("Sistema inicializado!", 0, 24);
    oled_write_no_clear("Monitorando...", 0, 36);
    sleep_ms(2000); // Exibe a mensagem por 2 segundos
}

int main() {
    setup();
    setup_device_id();
    int temperature = 0;


    while (true) {
        read_buttons();

        process_menu(&current_state, &temp_max, &temp_min);
        
        if (current_state == STATE_MONITORING){
            dht22_read(&temperature);
            check_temperature(&temperature);
        }

        sleep_ms(100);
    }

    return 0;
}
