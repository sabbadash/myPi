#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define LED_PATH "/sys/class/leds/ACT/"
#define TRIGGER_FILE "trigger"
#define BRIGHTNESS_FILE "brightness"

volatile int running = 1;

// обработчик сигнала для выхода
void handle_signal(int sig) {
    running = 0;
    printf("\n программа завершается -_-_-_- лол\n");
}

// функция для записи строки в файл
int write_to_file(const char *filename, const char *value) {
    FILE *fp;
    char full_path[256];
    
    snprintf(full_path, sizeof(full_path), "%s%s", LED_PATH, filename);
    fp = fopen(full_path, "w");
    if (fp == NULL) {
        printf("не выходит открыть файл: %s\n", full_path);
        return -1;
    }
    
    fprintf(fp, "%s", value);
    fclose(fp);
    return 0;
}

int main(void) {
    // регистрация обработчика сигнала
    signal(SIGINT, handle_signal);
    
    //отключаем стандартный триггер для управления вручную
    if (write_to_file(TRIGGER_FILE, "none") != 0) {
        printf("ошибка установки триггера. программа запущена не от имени root.\n");
        return 1;
    }
    
    printf("программа светодиода запущена.\n");
    printf("светодиод будет мигать каждую секунду. Ctrl+C для выхода.\n");
    
    // мигание светодиодом
    while (running) {
        // включение светодиод
        write_to_file(BRIGHTNESS_FILE, "1");
        printf("включен\n");
        sleep(1);
        
        // выключение светодиод
        write_to_file(BRIGHTNESS_FILE, "0");
        printf("выключен\n");
        sleep(1);
    }
    
    // восстанавливаем стандартный триггер при выходе
    write_to_file(TRIGGER_FILE, "mmc0");
    printf("Программа завершена. Восстановлен стандартный режим работы светодиода.\n");
    
    return 0;
}
