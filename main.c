#include <include/FreeRTOSConfig.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include "pico/stdlib.h"

// Definição dos pinos
#define LED_RED_PIN     11
#define LED_GREEN_PIN   12
#define LED_BLUE_PIN    13

#define BUTTON_A_PIN    5
#define BUTTON_B_PIN    6

// Estados das tarefas
volatile bool ledTaskSuspended = false;
volatile bool buzzerTaskSuspended = false;

// Handles das tarefas
TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t buzzerTaskHandle = NULL;

// Fila para eventos dos botões
QueueHandle_t buttonQueue = NULL;

// Enum para eventos dos botões
enum ButtonEvent {
    BUTTON_A_PRESSED,
    BUTTON_B_PRESSED
};

// Tarefa do LED RGB
void ledTask(void *pvParameters) {
    uint8_t currentColor = 0; // 0: Vermelho, 1: Verde, 2: Azul
    
    while (true) {
        if (!ledTaskSuspended) {
            // Desliga todos os LEDs
            gpio_put(LED_RED_PIN, 0);
            gpio_put(LED_GREEN_PIN, 0);
            gpio_put(LED_BLUE_PIN, 0);
            
            // Acende o LED atual
            switch (currentColor) {
                case 0:
                    gpio_put(LED_RED_PIN, 1);
                    break;
                case 1:
                    gpio_put(LED_GREEN_PIN, 1);
                    break;
                case 2:
                    gpio_put(LED_BLUE_PIN, 1);
                    break;
            }
            
            // Avança para a próxima cor
            currentColor = (currentColor + 1) % 3;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Espera 1 segundo
    }
}

// Tarefa do Buzzer
void buzzerTask(void *pvParameters) {
    while (true) {
        if (!buzzerTaskSuspended) {
            // Emite um bipe
            gpio_put(BUZZER_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_put(BUZZER_PIN, 0);
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Espera 2 segundos entre bipes
    }
}

// Tarefa de tratamento dos botões
void buttonTask(void *pvParameters) {
    ButtonEvent event;
    
    while (true) {
        // Espera por um evento na fila
        if (xQueueReceive(buttonQueue, &event, portMAX_DELAY) {
            switch (event) {
                case BUTTON_A_PRESSED:
                    ledTaskSuspended = !ledTaskSuspended;
                    if (ledTaskSuspended) {
                        // Desliga todos os LEDs quando suspenso
                        gpio_put(LED_RED_PIN, 0);
                        gpio_put(LED_GREEN_PIN, 0);
                        gpio_put(LED_BLUE_PIN, 0);
                    }
                    break;
                    
                case BUTTON_B_PRESSED:
                    buzzerTaskSuspended = !buzzerTaskSuspended;
                    if (buzzerTaskSuspended) {
                        // Desliga o buzzer quando suspenso
                        gpio_put(BUZZER_PIN, 0);
                    }
                    break;
            }
        }
    }
}

// Callback de interrupção para os botões
void gpioCallback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    ButtonEvent event;
    
    if (gpio == BUTTON_A_PIN) {
        event = BUTTON_A_PRESSED;
    } else if (gpio == BUTTON_B_PIN) {
        event = BUTTON_B_PRESSED;
    } else {
        return;
    }
    
    // Envia o evento para a fila
    xQueueSendFromISR(buttonQueue, &event, &xHigherPriorityTaskWoken);
    
    // Se necessário, faz uma troca de contexto
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

int main() {
    stdio_init_all();
    
    // Inicializa os GPIOs
    gpio_init(LED_RED_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_init(BUZZER_PIN);
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    
    // Configura os GPIOs
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    
    // Habilita pull-up para os botões
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);
    
    // Cria a fila para eventos dos botões
    buttonQueue = xQueueCreate(10, sizeof(ButtonEvent));
    
    // Cria as tarefas
    xTaskCreate(ledTask, "LED Task", 256, NULL, 1, &ledTaskHandle);
    xTaskCreate(buzzerTask, "Buzzer Task", 256, NULL, 1, &buzzerTaskHandle);
    xTaskCreate(buttonTask, "Button Task", 256, NULL, 2, NULL);
    
    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpioCallback);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpioCallback);
    
    // Inicia o escalonador
    vTaskStartScheduler();
    
    // Nunca deveria chegar aqui
    while (true);
    return 0;
}