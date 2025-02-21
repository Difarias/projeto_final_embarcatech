#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "src/ssd1306.h"
#include "src/buzzer.h"
#include "projeto_final.pio.h"

#define I2C_PORT i2c1
#define OLED_ADDR 0x3C
#define SDA_PIN 14
#define SCL_PIN 15
#define BUZZER_PIN 10
#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define LED_PIN 13  // Definindo o pino 13 para o LED
#define PINO_MATRIZ 7
#define DEBOUNCE_TIME_MS 200 // Tempo de debouncing em milissegundos

#define TEMPO_BASE 1000000 // 1 segundo por clique no botão A

// Pinagem do Joystick
#define JOYSTICK_PINO_X 26
#define JOYSTICK_PINO_Y 27
#define JOYSTICK_BOTAO 22
#define RESOLUCAO_ADC 4096
#define CENTRO_ADC 2048
#define LIMITE_JOYSTICK 1000 // Limite para considerar movimento do joystick

ssd1306_t display;
volatile bool buzzer_active = false;
volatile uint64_t start_time;
volatile bool button_b_pressed = false;
volatile int tempo_espera = 0; // Tempo configurado pelo botão A
volatile bool tempo_definido = false;

static uint8_t matriz[25][3];  // Matriz para armazenar o estado de cada LED (R, G, B)
static PIO pio_matriz;         // Controlador PIO usado para a matriz de LEDs
static uint maquina;           // Máquina de estado PIO usada para controlar a matriz
static volatile int numero_atual = 0;

// Variáveis para debouncing
static uint64_t last_button_a_time = 0;
static uint64_t last_button_b_time = 0;

// Variáveis para o teste de reflexo
static int posicao_alvo_x = 2; // Posição inicial do ponto alvo (centro da matriz)
static int posicao_alvo_y = 2;
static int posicao_usuario_x = 2; // Posição inicial do ponto do usuário (centro da matriz)
static int posicao_usuario_y = 2;
static uint64_t tempo_resposta = 0; // Tempo de resposta do usuário
static bool teste_em_andamento = false;

// Função para piscar o LED
void piscar_led() {
    gpio_put(LED_PIN, 1); // Acende o LED
    sleep_ms(200);        // Mantém o LED aceso por 200ms
    gpio_put(LED_PIN, 0); // Apaga o LED
}

// Função para debouncing do botão A
bool debounce_button_a() {
    uint64_t current_time = time_us_64();
    if (current_time - last_button_a_time > DEBOUNCE_TIME_MS * 1000) {
        last_button_a_time = current_time;
        return true;
    }
    return false;
}

// Função para debouncing do botão B
bool debounce_button_b() {
    uint64_t current_time = time_us_64();
    if (current_time - last_button_b_time > DEBOUNCE_TIME_MS * 1000) {
        last_button_b_time = current_time;
        return true;
    }
    return false;
}

void button_a_callback() {
    if (!tempo_definido && debounce_button_a()) {
        tempo_espera += TEMPO_BASE;
        printf("Tempo ajustado: %d segundos\n", tempo_espera / 1000000);  // Imprime no console
        piscar_led(); // Pisca o LED para feedback visual
    }
}

void button_b_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_B_PIN && debounce_button_b()) {
        button_b_pressed = true;

        // Se o tempo ainda não foi definido, confirma o tempo
        if (!tempo_definido) {
            tempo_definido = true;
            start_time = time_us_64();
            printf("Tempo definido: %d segundos\n", tempo_espera / 1000000);
        }

        // Se o alarme está ativo, interrompe o buzzer
        if (buzzer_active) {
            buzzer_active = false;
            pwm_set_gpio_level(BUZZER_PIN, 0);
        }
    }
}

void emitir_alerta() {
    ssd1306_fill(&display, false);
    ssd1306_draw_string(&display, "Pausa!", 40, 20);
    ssd1306_draw_string(&display, "Pressione B", 20, 40);
    ssd1306_send_data(&display);

    buzzer_active = true;
    beep(BUZZER_PIN, 15000); // Toca o buzzer

    while (!button_b_pressed) {
        sleep_ms(100);
    }

    start_time = time_us_64();
    button_b_pressed = false;
}

void inicializar_matriz(uint pino) {
    uint offset = pio_add_program(pio0, &matriz_led_program); // Adiciona o programa PIO
    pio_matriz = pio0; // Usa o controlador PIO0

    maquina = pio_claim_unused_sm(pio_matriz, false); // Obtém uma máquina de estado livre
    if (maquina < 0) {
        pio_matriz = pio1; // Se não houver máquinas livres no PIO0, tenta no PIO1
        maquina = pio_claim_unused_sm(pio_matriz, true);
    }

    // Inicializa o programa PIO na máquina de estado
    matriz_led_program_init(pio_matriz, maquina, offset, pino, 800000.f);
}

// Função para limpar a matriz (todos os LEDs apagados)
void limpar_matriz() {
    for (int i = 0; i < 25; i++) {
        matriz[i][0] = 0; // Vermelho
        matriz[i][1] = 0; // Verde
        matriz[i][2] = 0; // Azul
    }
}

// Função para enviar os dados da matriz para a máquina de estado PIO
void atualizar_matriz() {
    for (int i = 0; i < 25; i++) {
        pio_sm_put_blocking(pio_matriz, maquina, matriz[i][1]); // Verde
        pio_sm_put_blocking(pio_matriz, maquina, matriz[i][0]); // Vermelho
        pio_sm_put_blocking(pio_matriz, maquina, matriz[i][2]); // Azul
    }
}

// Função para desenhar um ponto na matriz de LEDs
void desenhar_ponto(int x, int y, int cor) {
    int indice = y * 5 + x;
    if (cor == 0) { // Vermelho
        matriz[indice][0] = 15;
    } else if (cor == 1) { // Azul
        matriz[indice][2] = 15;
    }
}

// Função para mover o ponto alvo aleatoriamente
void mover_ponto_alvo() {
    posicao_alvo_x = rand() % 5;
    posicao_alvo_y = rand() % 5;
}

// Função para ler o joystick
void ler_joystick(int *x, int *y) {
    adc_select_input(1); // Seleciona o eixo X
    int x_raw = adc_read(); // Lê o valor do eixo X
    adc_select_input(0); // Seleciona o eixo Y
    int y_raw = adc_read(); // Lê o valor do eixo Y

    // Mapeia os valores do joystick para movimento na matriz
    if (x_raw < CENTRO_ADC - LIMITE_JOYSTICK) *x = 1; // Movimento para a direita
    else if (x_raw > CENTRO_ADC + LIMITE_JOYSTICK) *x = -1; // Movimento para a esquerda
    else *x = 0; // Sem movimento

    if (y_raw < CENTRO_ADC - LIMITE_JOYSTICK) *y = 1; // Movimento para cima
    else if (y_raw > CENTRO_ADC + LIMITE_JOYSTICK) *y = -1; // Movimento para baixo
    else *y = 0; // Sem movimento
}

// Função principal do teste de reflexo
void teste_reflexo() {
    teste_em_andamento = true;
    mover_ponto_alvo(); // Move o ponto alvo para uma posição aleatória

    uint64_t inicio_teste = time_us_64();
    while (true) {
        // Lê o joystick
        int movimento_x, movimento_y;
        ler_joystick(&movimento_x, &movimento_y);

        // Atualiza a posição do usuário com base no joystick
        posicao_usuario_x += movimento_x;
        posicao_usuario_y += movimento_y;

        // Limita a posição do usuário aos limites da matriz
        if (posicao_usuario_x < 0) posicao_usuario_x = 0;
        if (posicao_usuario_x > 4) posicao_usuario_x = 4;
        if (posicao_usuario_y < 0) posicao_usuario_y = 0;
        if (posicao_usuario_y > 4) posicao_usuario_y = 4;

        // Limpa a matriz e desenha os pontos
        limpar_matriz();
        desenhar_ponto(posicao_alvo_x, posicao_alvo_y, 0); // Desenha o ponto alvo em vermelho
        desenhar_ponto(posicao_usuario_x, posicao_usuario_y, 1); // Desenha o ponto do usuário em azul
        atualizar_matriz(); // Atualiza a matriz com os novos desenhos

        // Verifica se o usuário alcançou o ponto alvo
        if (posicao_usuario_x == posicao_alvo_x && posicao_usuario_y == posicao_alvo_y) {
            tempo_resposta = time_us_64() - inicio_teste;
            printf("Tempo de resposta: %d ms\n", (int)(tempo_resposta / 1000));
            break;
        }

        sleep_ms(100); // Pequeno delay para evitar uso excessivo da CPU
    }

    teste_em_andamento = false;
}

int main() {
    stdio_init_all();
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Inicializa o ADC para o joystick
    adc_init();
    adc_gpio_init(JOYSTICK_PINO_X);
    adc_gpio_init(JOYSTICK_PINO_Y);

    inicializar_matriz(PINO_MATRIZ);
    limpar_matriz();

    ssd1306_init(&display, 128, 64, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&display);
    ssd1306_fill(&display, false);
    ssd1306_send_data(&display);

    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &button_b_callback);

    pwm_init_buzzer(BUZZER_PIN);

    // Inicializar GPIO 13 para o LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);  // Garantir que o LED esteja apagado inicialmente

    ssd1306_fill(&display, false);
    ssd1306_draw_string(&display, "Pressione A p/ setar", 5, 10);
    ssd1306_draw_string(&display, "o tempo do alarme", 10, 30);
    ssd1306_send_data(&display);

    // Loop principal
    while (true) {
        // Modo de configuração do tempo
        if (!tempo_definido) {
            // Detecta o clique do botão A
            if (gpio_get(BUTTON_A_PIN) == 0) { // Verifica se o botão A está pressionado (GND)
                button_a_callback();
            }

            // Atualiza o display com o tempo selecionado
            ssd1306_fill(&display, false);
            char msg[20];
            snprintf(msg, sizeof(msg), "Tempo: %d s", tempo_espera / 1000000);
            ssd1306_draw_string(&display, "Config Alarme", 10, 10);
            ssd1306_draw_string(&display, msg, 20, 30);
            ssd1306_send_data(&display);

            // Se o botão B for pressionado, confirma o tempo e inicia o contador
            if (button_b_pressed) {
                tempo_definido = true;
                start_time = time_us_64();
                button_b_pressed = false;

                // Exibe a mensagem "Contador iniciado!"
                ssd1306_fill(&display, false);
                ssd1306_draw_string(&display, "Contador", 40, 20);
                ssd1306_draw_string(&display, "iniciado!", 30, 40);
                ssd1306_send_data(&display);
                sleep_ms(2000); // Mostra a mensagem por 2 segundos
            }
        }
        // Modo de contagem do tempo
        else {
            if (time_us_64() - start_time >= tempo_espera) {
                emitir_alerta(); // Toca o alarme
                teste_reflexo(); // Inicia o teste de reflexo
                tempo_definido = false; // Reseta o tempo para permitir nova configuração
                tempo_espera = 0; // Reseta o tempo de espera
            }
        }

        sleep_ms(100); // Pequeno delay para evitar uso excessivo da CPU
    }
}