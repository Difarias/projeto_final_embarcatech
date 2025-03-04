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

// Definições de constantes
#define I2C_PORT i2c1
#define OLED_ADDR 0x3C
#define SDA_PIN 14
#define SCL_PIN 15
#define BUZZER_PIN 10
#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define LED_PIN 13  // Pino do LED
#define PINO_MATRIZ 7
#define DEBOUNCE_TIME_MS 200 // Tempo de debouncing em milissegundos
#define TEMPO_BASE 5000000 // 5 segundo por clique no botão A

// Pinagem do Joystick
#define JOYSTICK_PINO_X 27
#define JOYSTICK_PINO_Y 26
#define JOYSTICK_BOTAO 22
#define RESOLUCAO_ADC 4096
#define CENTRO_ADC 2048
#define LIMITE_JOYSTICK 100 // Limite para considerar movimento do joystick

// Variáveis globais
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

// Protótipos das funções
void exibir_acertos(int acertos);
void piscar_led();
bool debounce_button_a();
bool debounce_button_b();
void button_a_callback();
void button_b_callback(uint gpio, uint32_t events);
void emitir_alerta();
void inicializar_matriz(uint pino);
void limpar_matriz();
void atualizar_linha(int linha);
void atualizar_matriz();
void desenhar_ponto(int x, int y, int cor, int intensidade);
void alongamento_guiado();
void descansar_olhos();
void animacao_piscar();
void animacao_final();
void ler_joystick(uint16_t *x, uint16_t *y);
void mover_ponto_alvo();
void mapear_joystick_para_matriz(uint16_t x_raw, uint16_t y_raw, int *movimento_x, int *movimento_y);
void teste_reflexo();

// Função principal
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
    ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
    ssd1306_draw_string(&display, "Pressione A", 10, 10);
    ssd1306_draw_string(&display, "config alarme", 10, 30);
    ssd1306_send_data(&display);

    while (gpio_get(BUTTON_A_PIN) || !gpio_get(BUTTON_A_PIN)) {
        if (!gpio_get(BUTTON_A_PIN)) {  // Se o botão for pressionado (nível baixo)
            while (!gpio_get(BUTTON_A_PIN)) {  // Espera ser solto
                sleep_ms(50);
            }
            break;  // Sai do loop quando o botão for pressionado e depois solto
        }
        sleep_ms(50);
    }
        
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
            ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
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
                ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
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

// Implementações das funções

void exibir_acertos(int acertos) {
    ssd1306_fill(&display, false); // Limpa o display
    ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
    char msg[20];
    snprintf(msg, sizeof(msg), "Acertos: %d", acertos);
    ssd1306_draw_string(&display, msg, 20, 20); // Exibe a mensagem no display
    ssd1306_send_data(&display);
}

void piscar_led() {
    gpio_put(LED_PIN, 1); // Acende o LED
    sleep_ms(200);        // Mantém o LED aceso por 200ms
    gpio_put(LED_PIN, 0); // Apaga o LED
}

bool debounce_button_a() {
    uint64_t current_time = time_us_64();
    if (current_time - last_button_a_time > DEBOUNCE_TIME_MS * 1000) {
        last_button_a_time = current_time;
        return true;
    }
    return false;
}

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
            ssd1306_fill(&display, false);
            ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
            ssd1306_draw_string(&display, "Definido", 40, 20);
            ssd1306_draw_string(&display, "Aguarde", 20, 40);
            ssd1306_send_data(&display);
            tempo_definido = true;
            start_time = time_us_64();
            printf("Tempo definido: %d segundos\n", tempo_espera / 1000000);
        }

        // Se o alarme está ativo, interrompe o buzzer
        if (buzzer_active) {
            buzzer_active = false;
            pwm_set_gpio_level(BUZZER_PIN, 0);

            // Exibe a mensagem no OLED
            ssd1306_fill(&display, false);
            ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
            ssd1306_draw_string(&display, "Alarme", 40, 20);
            ssd1306_draw_string(&display, "desligado", 20, 35);
            ssd1306_draw_string(&display, "Aguarde", 30, 50);
            ssd1306_send_data(&display);

            // Imprime no monitor serial que o alarme foi pausado
            printf("Alarme pausado pelo botão B\n");
        }
    }
}

void emitir_alerta() {
    ssd1306_fill(&display, false);
    ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
    ssd1306_draw_string(&display, "Pausa!", 40, 20);
    ssd1306_draw_string(&display, "Pressione B", 20, 40);
    ssd1306_send_data(&display);

    buzzer_active = true;
    beep(BUZZER_PIN, 15000); // Toca o buzzer

    printf("Alarme emitido! Aguardando interrupção...\n");

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

void limpar_matriz() {
    for (int i = 0; i < 25; i++) {
        matriz[i][0] = 0; // Vermelho
        matriz[i][1] = 0; // Verde
        matriz[i][2] = 0; // Azul
    }
}

void atualizar_linha(int linha) {
    for (int coluna = 0; coluna < 5; coluna++) {
        int indice = linha * 5 + coluna; // Calcula o índice do LED na matriz
        pio_sm_put_blocking(pio_matriz, maquina, matriz[indice][1]); // Verde
        pio_sm_put_blocking(pio_matriz, maquina, matriz[indice][0]); // Vermelho
        pio_sm_put_blocking(pio_matriz, maquina, matriz[indice][2]); // Azul
    }
}

void atualizar_matriz() {
    for (int linha = 0; linha < 5; linha++) {
        atualizar_linha(linha); // Atualiza uma linha inteira de cada vez
    }
}

void desenhar_ponto(int x, int y, int cor, int intensidade) {
    y = 4 - y; // Inverte o eixo Y (0 → 4, 1 → 3, etc.)

    int indice;
    if (y % 2 == 0) {
        indice = y * 5 + x;
    } else {
        indice = y * 5 + (4 - x);
    }

    if (indice >= 0 && indice < 25) {
        if (cor == 0) { // Vermelho
            matriz[indice][0] = intensidade;
        } else if (cor == 1) { // Verde
            matriz[indice][1] = intensidade;
        } else if (cor == 2) { // Azul
            matriz[indice][2] = intensidade;
        }
    } else {
        printf("Erro: Índice fora dos limites! x: %d, y: %d, indice: %d\n", x, y, indice);
    }
}

void animacao_piscar() {
    for (int i = 0; i < 3; i++) { // Repete a animação 3 vezes
        limpar_matriz();
        for (int j = 0; j < 25; j++) {
            matriz[j][1] = 128; // Acende todos os LEDs em verde
        }
        atualizar_matriz();
        sleep_ms(200); // Mantém os LEDs acesos por 200ms

        limpar_matriz();
        atualizar_matriz();
        sleep_ms(200); // Mantém os LEDs apagados por 200ms
    }
}

void alongamento_guiado() {
    // Lista de alongamentos com instruções curtas
    char *instrucoes[4] = {
        "Se alongue",            // 1º exercício
        "Gire ombros",           // 2º exercício
        "Alongue pescoço",       // 3º exercício
        "Pisque olhos"           // 4º exercício
    };

    for (int i = 0; i < 4; i++) {
        // Limpa o display e desenha o retângulo
        ssd1306_fill(&display, false); // Limpa o buffer
        ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
        ssd1306_draw_string(&display, instrucoes[i], 17, 20); // Centraliza a mensagem
        ssd1306_send_data(&display); // Envia os dados para o display

        // Inicia a contagem regressiva de 10 segundos
        int tempo_restante = 10;
        while (tempo_restante > 0) {
            // Atualiza o display com o tempo restante
            char msg[20];
            snprintf(msg, sizeof(msg), "Tempo: %d", tempo_restante); // Remove o "s" para evitar sobreposição
            ssd1306_fill(&display, false); // Limpa o buffer
            ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Redesenha o retângulo
            ssd1306_draw_string(&display, instrucoes[i], 17, 20); // Redesenha a instrução
            ssd1306_draw_string(&display, msg, 40, 40); // Exibe o tempo restante
            ssd1306_send_data(&display); // Envia os dados para o display

            sleep_ms(1000); // Aguarda 1 segundo
            tempo_restante--; // Decrementa o tempo restante
        }

        // Feedback sonoro ao final do alongamento
        beep(BUZZER_PIN, 500); // Toca um beep de 500ms
        sleep_ms(500); // Aguarda 500ms para evitar repetição
        pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o buzzer

        // Exibe mensagem para confirmar o alongamento
        ssd1306_fill(&display, false); // Limpa o buffer
        ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
        ssd1306_draw_string(&display, "Pressione o", 20, 20);
        ssd1306_draw_string(&display, "joystick", 30, 35);
        ssd1306_send_data(&display); // Envia os dados para o display

        // Aguarda a confirmação do usuário com o joystick
        bool confirmado = false;
        while (!confirmado) {
            uint16_t x_raw, y_raw;
            ler_joystick(&x_raw, &y_raw);

            int movimento_x, movimento_y;
            mapear_joystick_para_matriz(x_raw, y_raw, &movimento_x, &movimento_y);

            // Confirma se o usuário moveu o joystick em qualquer direção
            if (movimento_x != 0 || movimento_y != 0) {
                confirmado = true;
                beep(BUZZER_PIN, 200); // Feedback sonoro
                animacao_piscar(); // Confirma na matriz de LEDs
            }

            sleep_ms(100); // Pequeno delay para evitar uso excessivo da CPU
        }

        // Limpa a matriz de LEDs após a confirmação
        limpar_matriz();
        atualizar_matriz();
        sleep_ms(500); // Pequena pausa entre os alongamentos
    }

    // Exibe mensagem de conclusão
    ssd1306_fill(&display, false); // Limpa o buffer
    ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
    ssd1306_draw_string(&display, "Alongamentos", 20, 20);
    ssd1306_draw_string(&display, "finalizados!", 20, 35);
    ssd1306_send_data(&display); // Envia os dados para o display
    sleep_ms(2000); // Mostra a mensagem por 2 segundos
}

void descansar_olhos() {
    limpar_matriz();
    atualizar_matriz();

    int tempo_restante = 20; // Tempo inicial de 20 segundos

    while (tempo_restante > 0) {
        // Atualiza o display com o tempo restante
        ssd1306_fill(&display, false); // Limpa o buffer
        ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
        char msg[20];
        snprintf(msg, sizeof(msg), "Feche os olhos: %d", tempo_restante); // Remove o "s" para evitar sobreposição
        ssd1306_draw_string(&display, msg, 7, 25);
        ssd1306_send_data(&display); // Envia os dados para o display

        sleep_ms(1000); // Aguarda 1 segundo
        tempo_restante--; // Decrementa o tempo restante
    }

    // Beep simples para alertar o fim do tempo
    beep(BUZZER_PIN, 500); // Toca um beep de 500ms
    sleep_ms(500); // Aguarda 500ms para evitar repetição
    pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o buzzer

    // Exibe a mensagem final e retorna ao estado inicial
    ssd1306_fill(&display, false); // Limpa o buffer
    ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
    ssd1306_draw_string(&display, "Pausa", 15, 20);
    ssd1306_draw_string(&display, "Concluida", 10, 35);
    ssd1306_send_data(&display); // Envia os dados para o display
    sleep_ms(2000); // Mostra a mensagem por 2 segundos

    // Chama a função de alongamento guiado
    alongamento_guiado();
}

void animacao_final() {
    limpar_matriz(); // Apaga todos os LEDs da matriz

    for (int linha = 0; linha < 5; linha++) { // Itera sobre cada linha
        for (int coluna = 0; coluna < 5; coluna++) { // Itera sobre cada coluna da linha
            int indice = linha * 5 + coluna; // Calcula o índice do LED na matriz
            matriz[indice][0] = 128; // Vermelho (50% de brilho)
            matriz[indice][1] = 128; // Verde (50% de brilho)
            matriz[indice][2] = 0;  // Azul (desligado)
        }

        atualizar_matriz(); // Atualiza a matriz com a nova linha acesa
        beep(BUZZER_PIN, 500); // Toca o buzzer por 500ms
        sleep_ms(500); // Aguarda 500ms antes de acender a próxima linha
    }

    // Desliga o buzzer após a animação
    pwm_set_gpio_level(BUZZER_PIN, 0);

    // Chama a função para descansar os olhos
    descansar_olhos();
}

void ler_joystick(uint16_t *x, uint16_t *y) {
    adc_select_input(1); // Seleciona o eixo X (pino 27)
    *x = adc_read(); // Lê o valor do eixo X
    adc_select_input(0); // Seleciona o eixo Y (pino 26)
    *y = adc_read(); // Lê o valor do eixo Y
}

void mover_ponto_alvo() {
    posicao_alvo_x = rand() % 5; // Gera um número aleatório entre 0 e 4
    posicao_alvo_y = rand() % 5; // Gera um número aleatório entre 0 e 4
}

void mapear_joystick_para_matriz(uint16_t x_raw, uint16_t y_raw, int *movimento_x, int *movimento_y) {
    // Mapeia o eixo X (pino 27)
    if (x_raw < CENTRO_ADC - LIMITE_JOYSTICK) {
        *movimento_x = 1; // Movimento para a esquerda
    } else if (x_raw > CENTRO_ADC + LIMITE_JOYSTICK) {
        *movimento_x = -1; // Movimento para a direita
    } else {
        *movimento_x = 0; // Sem movimento
    }

    // Mapeia o eixo Y (pino 26)
    if (y_raw < CENTRO_ADC - LIMITE_JOYSTICK) {
        *movimento_y = 1; // Movimento para cima (eixo Y invertido)
    } else if (y_raw > CENTRO_ADC + LIMITE_JOYSTICK) {
        *movimento_y = -1; // Movimento para baixo (eixo Y invertido)
    } else {
        *movimento_y = 0; // Sem movimento
    }
}

void teste_reflexo() {
    teste_em_andamento = true;
    int acertos = 0; // Contador de acertos
    int meta_acertos = 10; // Meta de acertos
    uint64_t tempo_limite = 30000000; // 30s 
    bool tentar_novamente = true;

    while (tentar_novamente) {
        acertos = 0; // Reinicia o contador de acertos
        mover_ponto_alvo(); // Move o ponto alvo para uma posição aleatória

        // Exibe a meta de acertos antes de iniciar o teste
        ssd1306_fill(&display, false);
        ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
        ssd1306_draw_string(&display, "Meta 10ac", 10, 20);
        ssd1306_draw_string(&display, "Tempo 30s", 10, 35);
        ssd1306_send_data(&display);
        sleep_ms(3000); // Mostra a mensagem por 3 segundos

        // Exibe a nova mensagem antes de iniciar o teste de reflexo
        ssd1306_fill(&display, false);
        ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
        ssd1306_draw_string(&display, "Ache o", 40, 20);
        ssd1306_draw_string(&display, "ponto vermelho", 10, 35);
        ssd1306_send_data(&display);

        uint64_t inicio_teste = time_us_64();
        while (acertos < meta_acertos && (time_us_64() - inicio_teste) < tempo_limite) {
            // Lê o joystick
            uint16_t x_raw, y_raw;
            ler_joystick(&x_raw, &y_raw);

            // Mapeia os valores do joystick para a matriz
            int movimento_x, movimento_y;
            mapear_joystick_para_matriz(x_raw, y_raw, &movimento_x, &movimento_y);

            // Atualiza a posição do usuário com base no joystick
            posicao_usuario_x += movimento_x;
            posicao_usuario_y += movimento_y;

            // Limita a posição do usuário aos limites da matriz
            if (posicao_usuario_x < 0) posicao_usuario_x = 0;
            if (posicao_usuario_x > 4) posicao_usuario_x = 4;
            if (posicao_usuario_y < 0) posicao_usuario_y = 0;
            if (posicao_usuario_y > 4) posicao_usuario_y = 4;

            // Se o jogador alcançar o ponto alvo
            if (posicao_usuario_x == posicao_alvo_x && posicao_usuario_y == posicao_alvo_y) {
                acertos++;
                printf("Acerto %d! Movendo o alvo...\n", acertos);
                exibir_acertos(acertos); // Atualiza o display com a quantidade de acertos
                mover_ponto_alvo(); // Move o alvo para um novo local
            }

            // Limpa a matriz e desenha os pontos
            limpar_matriz();
            desenhar_ponto(posicao_alvo_x, posicao_alvo_y, 0, 128); // Desenha o ponto alvo em vermelho (50% de brilho)
            desenhar_ponto(posicao_usuario_x, posicao_usuario_y, 1, 128); // Desenha o ponto do usuário em azul (50% de brilho)
            atualizar_matriz(); // Atualiza a matriz com os novos desenhos

            sleep_ms(100); // Pequeno delay para evitar uso excessivo da CPU
        }

        // Verifica se o jogador atingiu a meta de acertos
        if (acertos >= meta_acertos) {
            // Acende os LEDs da matriz em amarelo (vermelho + verde)
            for (int i = 0; i < 25; i++) {
                matriz[i][0] = 128; // Vermelho
                matriz[i][1] = 128; // Verde
                matriz[i][2] = 0;  // Azul
            }
            atualizar_matriz();
            ssd1306_fill(&display, false);
            ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
            ssd1306_draw_string(&display, "Parabens!", 30, 20);
            ssd1306_draw_string(&display, "Meta alcancada", 10, 35);
            ssd1306_send_data(&display);
            sleep_ms(3000); // Mostra a mensagem por 3 segundos
            tentar_novamente = false; // Sai do loop de tentativas
            printf("Meta de 10 acertos alcançada!\n"); // Imprime no monitor serial
        } else {
            // Acende os LEDs da matriz em vermelho
            for (int i = 0; i < 25; i++) {
                matriz[i][0] = 128; // Vermelho
                matriz[i][1] = 0;   // Verde
                matriz[i][2] = 0;   // Azul
            }
            atualizar_matriz();
            ssd1306_fill(&display, false);
            ssd1306_rect(&display, 0, 0, 128, 64, true, false); // Desenha um retângulo
            ssd1306_draw_string(&display, "Tempo esgotado!", 10, 20);
            ssd1306_draw_string(&display, "Pressione B para", 10, 35);
            ssd1306_draw_string(&display, "tentar novamente", 10, 50);
            ssd1306_send_data(&display);

            // Aguarda a decisão do jogador
            while (true) {
                if (button_b_pressed) {
                    button_b_pressed = false; // Reseta o estado do botão
                    tentar_novamente = true; // Reinicia o teste
                    printf("Reiniciando o teste de reflexo...\n"); // Imprime no monitor serial
                    break;
                }
                sleep_ms(100); // Pequeno delay para evitar uso excessivo da CPU
            }
        }
    }

    printf("Teste finalizado!\n");
    teste_em_andamento = false;
    button_b_pressed = false; // Garante que o botão B não fique marcado
    tempo_definido = false; // Permite reconfigurar o tempo
    tempo_espera = 0; // Zera o tempo configurado

    animacao_final(); // Chama a animação final apenas após o sucesso ou desistência
}