#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/statvfs.h>

#define MAX_CPUS 256
#define MAX_NET 128

typedef unsigned long long  ull;

const char* proc_root(void) {
    const char* env = getenv("PROC_ROOT");
    return env ? env : "/proc";
}

double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

typedef struct {
    ull total, idle;
    int initialized;
    char name[16];
} Cpu;

Cpu last_cpu[MAX_CPUS];
int cpu_count = 0;

int find_cpu(const char* name) {
    for(int i = 0; i < cpu_count; ++i) {
        if(strcmp(name, last_cpu[i].name) == 0) {
            return i;
        }
    }
    if(cpu_count >= MAX_CPUS) {
        return -1;
    }
    
    last_cpu[cpu_count].total = 0;
    last_cpu[cpu_count].idle = 0;
    last_cpu[cpu_count].initialized = 0;
    strcpy(last_cpu[cpu_count].name, name);
    return cpu_count++;
}


void cpu(void) {
    char path[256];
    snprintf(path, sizeof(path), "%s/stat", proc_root());
    FILE* file = fopen(path, "r");
    if(!file) {
        printf("can't read cpu\n");
        return;
    }

    char line[256];
    while(fgets(line, sizeof(line), file)) {
        ull user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0, guest = 0, guest_nice = 0;
        char name[16];
        int n = sscanf(line, "%15s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", name, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice);
        if (n < 5) {
            continue;
        }
        if (strncmp(name, "cpu", 3) == 0) {
            ull total_now = user + nice + system + idle + iowait + irq + softirq + steal,
                idle_now = idle + iowait;
            int idx = find_cpu(name);
            if(idx < 0) {
                printf("can't read more about cpu");
                break;
            }

            if(last_cpu[idx].initialized == 0) {
                printf("%8s information is collecting...\n", name);
                last_cpu[idx].initialized = 1;
            }
            else {
                ull total_deta = total_now - last_cpu[idx].total,
                    idle_deta = idle_now - last_cpu[idx].idle;
                if (total_deta == 0) {
                    continue;
                }
                printf("%8s | 使用率: %.2f%%\n", name, (1 - idle_deta * 1.0 / (total_deta * 1.0)) * 100);
            }
            last_cpu[idx].total = total_now;
            last_cpu[idx].idle = idle_now;
        }
    }

    fclose(file);
}

void ram(void) {
    char path[256];
    snprintf(path, sizeof(path), "%s/meminfo", proc_root());
    FILE* file = fopen(path, "r");
    if(!file){
        printf("can't read ram\n");
        return;
    }
    char line[256];
    ull total = 0, available = 0;

    while(fgets(line, sizeof(line), file)) {
        char name[32];
        ull value = 0;
        int n = sscanf(line, "%31[^:]: %llu", name, &value);
        if(n < 2) {
            continue;
        }
        if(strncmp(name, "Mem", 3) == 0) {
            if(strcmp(name, "MemTotal") == 0){
                total = value;
            }
            else if(strcmp(name, "MemAvailable") == 0) {
                available = value;
            }
        }
    }
    if(total && available) {
        printf("     RAM | 使用率: %.2lf%%\n", (1 - available * 1.0 / (total * 1.0)) * 100);
    }

    fclose(file);
}

void print_speed(double speed) {
    if (speed >= 1024*1024) {
        printf("%.2lf MB/s", speed / 1024.0 / 1024.0);
    }
    else if (speed >= 1024) {
        printf("%.2lf KB/s", speed / 1024.0);
    }
    else {
        printf("%.2f B/s", speed);
    }
}


typedef struct {
    ull tx, rx;
    char name[32];
    int initialized;
    double time;
} Netdev;
int net_count = 0;

Netdev last_net[MAX_NET];
int get_net(char* name){
    for(int i = 0; i < net_count; ++i) {
        if(strcmp(name, last_net[i].name) == 0){
            return i;
        }
    }
    if(net_count >= MAX_NET){
        return -1;
    }
    last_net[net_count].tx = 0;
    last_net[net_count].rx = 0;
    strcpy(last_net[net_count].name, name);
    last_net[net_count].initialized = 0;
    last_net[net_count].time = now_seconds();
    return net_count++;
}

void netdev(void) {
    char path[256];
    snprintf(path, sizeof(path), "%s/net/dev", proc_root());
    char line[256];
    FILE* file = fopen(path, "r");
    double now = now_seconds();
    if(!file) {
        printf("can't read netdev\n");
        return;
    }

    char name[32];
    ull value[16];
    ull rx_now = 0, tx_now = 0;
    while(fgets(line, sizeof(line), file)) {
        int n = sscanf(line, " %31[^:]: %llu %llu %llu %llu %llu %llu %llu %llu %llu", name, &value[0], &value[1], &value[2], &value[3], &value[4], &value[5] , &value[6], &value[7], &value[8]);
        if (n < 10) {
            continue;
        }
        rx_now = value[0];
        tx_now = value[8];
        int idx = get_net(name);
        if(idx < 0) {
            printf("can't read more about netdev\n");
            break;
        }

        if(!last_net[idx].initialized) {
            printf("%7s information is collecting...\n", name);
            last_net[idx].initialized = 1;
        }
        else {
            ull rx_deta = rx_now - last_net[idx].rx,
                tx_deta = tx_now - last_net[idx].tx;
            double time_deta = now - last_net[idx].time;
            if(time_deta <= 0) {
                continue;
            }
            printf("%8s | 接收速度: ", name);
            print_speed(rx_deta / time_deta);
            printf(" | 传输速度: ");
            print_speed(tx_deta / time_deta);
            printf("\n");
        }
        last_net[idx].tx = tx_now;
        last_net[idx].rx = rx_now;
        last_net[idx].time = now;
    }
    fclose(file); 
}

void disk() {
    struct statvfs buf;
    if(statvfs("/", &buf) != 0) {
        printf("can't read disk\n");
        return;
    }
    ull total = buf.f_blocks * buf.f_frsize;
    ull free = buf.f_bfree * buf.f_frsize;
    ull used = total - free;
    printf("    DISK | 使用率: %.2lf%% | 已用/可用: %.2fGB/%.2fGB\n", (1 - free * 1.0 / (total * 1.0)) * 100, used/1024.0/1024.0/1024.0, free/1024.0/1024.0/1024.0);
}

void ports() {
    char path[256];
    snprintf(path, sizeof(path), "%s/net/tcp", proc_root());
    FILE* file1 = fopen(path, "r");
    snprintf(path, sizeof(path), "%s/net/tcp6", proc_root());
    FILE* file2 = fopen(path, "r");
    char line[256];
    if(file1) {
        printf("    PORT | TCP LISTEN:");
        while(fgets(line, sizeof(line), file1)) {
            char state[4];
            int port = 0;
            sscanf(line, "%*d: %*X:%X %*X:%*X %s", &port, state);
            if(port != 0 && strcmp(state, "0A") == 0) {
                printf(" %d", port);     
            }
        }
        printf("\n");
        fclose(file1);
    }
    if (file2){
        printf("    PORT | TCP IPv6 LISTEN:");
        while(fgets(line, sizeof(line), file2)) {
            char state[4];
            int port = 0;
            sscanf(line, "%*d: %*X:%X %*X:%*X %s", &port, state);
            if(port != 0 && strcmp(state, "0A") == 0) {
                printf(" %d", port);     
            }
        }
        printf("\n");
        fclose(file2);
    }
    if(!file1 && !file2) {
        printf("can't read ports\n");
        return;
    }
}


int main(int argc, char* argv[]) {
    int delay = 10;
    for(int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "-d") == 0) {
            if(i + 1 < argc) {
                delay = atoi(argv[++i]);
                if(delay <= 0) {
                    printf("刷新时间应为正数。\n");
                    return 1;
                }
            }
            else {
                printf("请提供刷新时间\n");
                return 1;
            }
        }
        else if(strcmp(argv[i], "-h") == 0) {
            printf("用法: %s [-d <刷新时间>] [-h]\n", argv[0]);
            return 0;
        }
        else {
            printf("未知参数: %s\n", argv[i]);
            return 1;
        }
    }
    
    while(1){


        cpu();
        ram();
        netdev();
        disk();
        ports();
        printf("----------------------------------------\n");

        sleep(delay);
    }



}
