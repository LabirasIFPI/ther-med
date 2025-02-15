#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "../utils/led_matrix_funcs.h"

#define BUZZER_PIN 21     // Definição do GPIO onde o buzzer passivo está conectado

volatile bool alarm_state = false;  // Define o ESTADO do alarme, para gerar o alarme em pulso
struct repeating_timer alarm_timer; // Temporizador do PULSO do alarme
bool alarm_active = false; // Define se o alarme está ativado

/**
 *  Função que liga/desliga os alarmes em um determinado tempo
 *  @param[in] t Temporizador que repete o pulso do alarme
 *  @return bool 
 */
bool alarm_toggle_callback(struct repeating_timer *t) {
    if (alarm_active) {
        alarm_state = !(alarm_state); // Alternância da variável de controle para geração de pulso

        // Realização do pulso dos LEDs e do buzzer
        led_matrix_colorize(alarm_state ? GRB_RED : GRB_BLACK);
        pwm_set_gpio_level(BUZZER_PIN, alarm_state ? 1000 : 0); 
    } else {
        pwm_set_gpio_level(BUZZER_PIN, 0);  // Garante que o buzzer fique desligado
    }

    return true;
}

/**
 * Inicializa o buzzer, que servirá para gerar alarmes em caso de leituras fora da faixa segura
 */
void buzzer_init() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);  // Define o pino como saída PWM
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice_num, 50000);  // Define a frequência
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
