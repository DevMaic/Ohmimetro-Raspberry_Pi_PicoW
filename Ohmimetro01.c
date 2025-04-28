/*
 * Por: Wilton Lacerda Silva
 *    Ohmímetro utilizando o ADC da BitDogLab
 *
 * 
 * Neste exemplo, utilizamos o ADC do RP2040 para medir a resistência de um resistor
 * desconhecido, utilizando um divisor de tensão com dois resistores.
 * O resistor conhecido é de 10k ohm e o desconhecido é o que queremos medir.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "pio_matrix.pio.h"
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28 // GPIO para o voltímetro
#define Botao_A 5  // GPIO para botão A
#define OUT_PIN 7
#define NUM_PIXELS 25

int R_conhecido = 10000;   // Resistor de 10k ohm
float R_x = 0.0;           // Resistor desconhecido
float ADC_VREF = 3.31;     // Tensão de referência do ADC
int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)

const char* resistor_colors[] = {
  "Preto", "Marrom", "Vermelho", "Laranja",
  "Amarelo", "Verde", "Azul", "Violeta",
  "Cinza", "Branco"
};
typedef struct {
  double r;
  double g;
  double b;
} Pixel;

Pixel resistorColors[10] = {
  {0.0, 0.0, 0.0}, {0.07, 0.03, 0.015}, {1.0, 0.0, 0.0}, {1.0, 0.31, 0.0}, {1.0, 0.78, 0.0},
  {0.0, 0.59, 0.0}, {0.0, 0.0, 0.78}, {0.63, 1.0, 1.0}, {0.39, 0.39, 0.39}, {0.0, 0.0, 0.0}
};

const double E24[] = {
  10, 11, 12, 13, 15, 16, 18, 20,
  22, 24, 27, 30, 33, 36, 39, 43,
  47, 51, 56, 62, 68, 75, 82, 91
};
const int E24_SIZE = 24;

double findNearestResistor(double resistanceRead) {
  double nearestResistance = 0;
  double minDifference = 1000000;  // Um valor inicial bem alto

  for (int i = -1; i <= 6; i++) {
    double factor = pow(10, i);
    for (int j = 0; j < E24_SIZE; j++) {
      double E24resistance = E24[j] * factor;
      double difference = fabs(E24resistance - resistanceRead);

      if (difference < minDifference) {
        minDifference = difference;
        nearestResistance = E24resistance;
      }
    }
  }

  return nearestResistance;
}

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

//rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double b, double r, double g)
{
  unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

Pixel desenho[25] = {
  {0.8, 0.3, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 0.0},
  {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
  {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
  {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
  {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}
};

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(uint32_t valor_led, PIO pio, uint sm) {
  for (int16_t i = 0; i < NUM_PIXELS; i++) {
    valor_led = matrix_rgb(
      desenho[24-i].b,  // azul
      desenho[24-i].r,  // vermelho
      desenho[24-i].g   // verde
    );
    pio_sm_put_blocking(pio, sm, valor_led);
  }
}

void acende_led_unicor(int led_index, double r, double g, double b, uint32_t valor_led, PIO pio, uint sm) {
  for (int16_t i = 0; i < NUM_PIXELS; i++) {
    if (i == led_index) {
      // Se for o LED que queremos, acende com as cores passadas
      valor_led = matrix_rgb(b, r, g);
    } 
    // else {
    //   // Os outros ficam apagados (0.0)
    //   valor_led = matrix_rgb(0.0, 0.0, 0.0);
    // }
    pio_sm_put_blocking(pio, sm, valor_led);
  }
}

void set_pixel_color(int led_index, double r, double g, double b) {
    if (led_index >= 0 && led_index < NUM_PIXELS) {
        desenho[led_index].r = r;
        desenho[led_index].g = g;
        desenho[led_index].b = b;
    }
}

int main() {
  for (int i = 0; i < NUM_PIXELS; i++) {
    desenho[i].r = 0.0;
    desenho[i].g = 0.0;
    desenho[i].b = 0.0;
  }
  uint32_t valor_led; // Variável para armazenar o valor do LED
  PIO pio = pio0;
  set_sys_clock_khz(128000, false);
  //configurações da PIO
  uint offset = pio_add_program(pio, &pio_matrix_program);
  uint sm = pio_claim_unused_sm(pio, true);
  pio_matrix_program_init(pio, sm, offset, OUT_PIN);

  // Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  // Aqui termina o trecho para modo BOOTSEL com botão B

  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA);                                        // Pull up the data line
  gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
  ssd1306_t ssd;                                                // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd);                                         // Configura o display
  ssd1306_send_data(&ssd);                                      // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init();
  adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

  float tensao;
  char str_x[5]; // Buffer para armazenar a string
  char str_y[5]; // Buffer para armazenar a string

  bool cor = true;
  while(true) {
    adc_select_input(2); // Seleciona o ADC do pino 28 como entrada analógica

    float soma = 0.0f;
    for (int i = 0; i < 500; i++) {
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma / 500.0f;

    // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
    R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);
    double aux = R_x; // Armazena o valor de R_x para comparação
    aux = findNearestResistor(aux); // Encontra o resistor mais próximo
    int magnitude; // Calcula a magnitude do resistor
    for(magnitude=0;aux>=100; magnitude++) {
      aux = aux / 10; // Divide por 10 até que o valor seja menor que 100
    }
    printf("%d %d\n", (int)aux/10, ((int)aux)%10);

    set_pixel_color(0, resistorColors[(int)aux/10].r, resistorColors[(int)aux/10].g, resistorColors[(int)aux/10].b);
    set_pixel_color(1, resistorColors[(int)aux%10].r, resistorColors[(int)aux%10].g, resistorColors[(int)aux%10].b);
    set_pixel_color(2, resistorColors[magnitude].r, resistorColors[magnitude].g, resistorColors[magnitude].b);
    desenho_pio(valor_led, pio, sm);

    sprintf(str_x, "%1.0f", media); // Converte o inteiro em string
    sprintf(str_y, "%1.0f", R_x);   // Converte o float em string

    // cor = !cor;
    //  Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor);                          // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
    ssd1306_line(&ssd, 20, 7, 105, 7, cor);           // Desenha uma linha
    ssd1306_line(&ssd, 20, 7, 20, 25, cor);          // Desenha uma linha
    ssd1306_line(&ssd, 20, 25, 105, 25, cor);           // Desenha uma linha
    ssd1306_line(&ssd, 105, 25, 105, 7, cor);           // Desenha uma linha
    ssd1306_line(&ssd, 10, 16, 20, 16, cor);           // Desenha uma linha
    ssd1306_line(&ssd, 105, 16, 115, 16, cor);           // Desenha uma linha
    ssd1306_rect(&ssd, 7, 30, 10, 19, cor, cor);      // Desenha um retângulo
    ssd1306_rect(&ssd, 7, 45, 10, 19, cor, cor);      // Desenha um retângulo
    ssd1306_rect(&ssd, 7, 60, 10, 19, cor, cor);      // Desenha um retângulo
    ssd1306_rect(&ssd, 7, 90, 10, 19, cor, cor);      // Desenha um retângulo

    // ssd1306_draw_string(&ssd, "R", 33, 13); // Desenha uma string
    // ssd1306_draw_string(&ssd, "G", 48, 13); // Desenha uma string
    // ssd1306_draw_string(&ssd, "B", 63, 13); // Desenha uma string

    ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // Desenha uma linha
    ssd1306_draw_string(&ssd, "", 8, 6); // Desenha uma string
    ssd1306_draw_string(&ssd, "", 20, 16);  // Desenha uma string
    ssd1306_draw_string(&ssd, "  Ohmimetro", 10, 29);  // Desenha uma string
    ssd1306_draw_string(&ssd, "ADC", 13, 41);          // Desenha uma string
    ssd1306_draw_string(&ssd, "Resisten.", 50, 41);    // Desenha uma string
    ssd1306_line(&ssd, 44, 37, 44, 60, cor);           // Desenha uma linha vertical
    ssd1306_draw_string(&ssd, str_x, 8, 52);           // Desenha uma string
    ssd1306_draw_string(&ssd, str_y, 59, 52);          // Desenha uma string
    ssd1306_send_data(&ssd);                           // Atualiza o display
    sleep_ms(700);

    // Atualiza o conteúdo do display com cores do resistor lido
    ssd1306_fill(&ssd, !cor);
    ssd1306_draw_string(&ssd, "Resistor:", 8, 6);
    ssd1306_draw_string(&ssd, resistor_colors[(int)aux/10], 8, 16);
    ssd1306_draw_string(&ssd, resistor_colors[(int)aux%10], 8, 26);
    ssd1306_draw_string(&ssd, resistor_colors[magnitude], 8, 36);
    ssd1306_send_data(&ssd);

    sleep_ms(700);
  }
}