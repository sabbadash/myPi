#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include <pthread.h>

#define LED_PATH "/sys/class/leds/ACT/"
#define TRIGGER_FILE "trigger"
#define BRIGHTNESS_FILE "brightness"
#define MONITORING_INTERVAL_SEC 5  // интервал сбора метрик
#define LOG_FILE "system_metrics.csv"

FILE *log_fp = NULL;
volatile int running = 1;
pthread_t led_thread_id;

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

float get_cpu_temperature() {
    FILE *temp_file;
    float temp = 0.0;
    char buffer[100];
    
    // температура из системного файла
    temp_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (temp_file) {
        if (fgets(buffer, sizeof(buffer), temp_file) != NULL) {
            temp = atoi(buffer) / 1000.0; // миллиградусы в градусы
        }
        fclose(temp_file);
    }
    
    return temp;
}

float get_memory_usage() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        float total_ram = (float)info.totalram;
        float free_ram = (float)info.freeram;
        return 100.0 * (1.0 - (free_ram / total_ram));
    }
    return 0.0;
}

float get_disk_usage() {
    FILE *df_pipe;
    char buffer[256];
    float usage = 0.0;
    
    // проверка использования диска системной командой df
    df_pipe = popen("df -h / | tail -1 | awk '{print $5}'", "r");
    if (df_pipe) {
        if (fgets(buffer, sizeof(buffer), df_pipe)) {
            usage = atof(buffer);
        }
        pclose(df_pipe);
    }
    
    return usage;
}

//поток для мигания светодиодом
void *led_thread(void *arg) {
    while (running) {
        // Включаем светодиод
        write_to_file(BRIGHTNESS_FILE, "1");
        usleep(100000); // 100 мс
        
        // Выключаем светодиод
        write_to_file(BRIGHTNESS_FILE, "0");
        usleep(100000); // 100 мс
    }
    return NULL;
}

int main(void) {
    // регистрация обработчика сигнала
    signal(SIGINT, handle_signal);
    log_fp = fopen(LOG_FILE, "a");
    if (log_fp) {
        fseek(log_fp, 0, SEEK_END);
        if (ftell(log_fp) == 0) {
            fprintf(log_fp, "timestamp,cpu_temp,cpu_usage,memory_usage,disk_usage\n");
        }
    }

    //отключаем стандартный триггер для управления вручную
    if (write_to_file(TRIGGER_FILE, "none") != 0) {
        printf("ошибка установки триггера. программа запущена не от имени root.\n");
        return 1;
    }
    
    printf("программа светодиода запущена.\n");
    printf("светодиод будет мигать каждую секунду.  Ctrl+C для выхода.\n");
    
    // отдельный поток для мигания светодиодом
    if (pthread_create(&led_thread_id, NULL, led_thread, NULL) != 0) {
        printf("error создания потока для светодиода\n");
        write_to_file(TRIGGER_FILE, "mmc0"); // Восстановить триггер
        return 1;
    }
    
    // основной цикл мониторинга
    while (running) {
        // получение системных показателей
        float cpu_temp = get_cpu_temperature();
        float memory_usage = get_memory_usage();
        float disk_usage = get_disk_usage();
        
        // вывод информации
        printf("\033[2J\033[H"); // очистка экрана и перемещение курсора в начало
        printf("=== система Raspberry Pi ===\n");
        printf("температура CPU: %.2f°C\n", cpu_temp);
        printf("память: %.2f%%\n", memory_usage);
        printf("диска: %.2f%%\n", disk_usage);
        printf("светодиод: мигает\n");
        printf("======================================\n");
        printf("ctrl+c для выхода\n");
        
        if (log_fp) {
            time_t now = time(NULL);
            fprintf(log_fp, "%ld,%.2f,%.2f,%.2f,\n", 
                    now, cpu_temp, memory_usage, disk_usage);
            fflush(log_fp);
        }

        // проверка критических значений и изменение частоты мигания можно добавить здесь
        
        // ожидание до следующего обновления статистики
        sleep(MONITORING_INTERVAL_SEC);
    }
    
    // ожидание завершения потока светодиода
    pthread_join(led_thread_id, NULL);
    
    // восстанавливаем стандартный триггер при выходе
    write_to_file(TRIGGER_FILE, "mmc0");
    if (log_fp) fclose(log_fp);
    printf("программа завершена.\n");
    
    return 0;
}
