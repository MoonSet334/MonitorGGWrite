#include <cstdio>
#include <cstring>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <android/log.h>
#include <sys/ptrace.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#endif

#define LOG_TAG "GGPtraceHook"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define LOG_FILE_PATH "/sdcard/gg_ptrace.log"

static void get_log_time(char* buffer, size_t size) {
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &tm_info);
}

static void log_ptrace_write(uintptr_t address, uint32_t value) {
    char time_buf[32];
    get_log_time(time_buf, sizeof(time_buf));

    FILE* log_file = fopen(LOG_FILE_PATH, "a");
    if (!log_file) {
        LOGE("无法打开日志: %s", LOG_FILE_PATH);
        return;
    }

    fprintf(log_file, "[%s] Address: 0x%016lX Value: %u\n", time_buf, address, value);
    fclose(log_file);

    LOGD("[%s] Address: 0x%016lX Value: %u", time_buf, address, value);
}

long my_syscall(long number, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    static __thread uintptr_t last_peek_addr = 0;
    static __thread uint64_t last_peek_value = 0;

    if (number == 117) {
        long request = arg1;

        if (request == PTRACE_PEEKTEXT || request == PTRACE_PEEKDATA) {
            last_peek_addr = (uintptr_t)arg3;
        }
    }

    // 执行原始 syscall
    long result = syscall(number, arg1, arg2, arg3, arg4, arg5, arg6);

    if (number == 117) {
        long request = arg1;

        if ((request == PTRACE_PEEKTEXT || request == PTRACE_PEEKDATA) && result != -1) {
            last_peek_value = (uint64_t)result;
        }
        else if (request == PTRACE_POKETEXT || request == PTRACE_POKEDATA) {
            uintptr_t poke_addr = (uintptr_t)arg3;
            uint32_t new_value = (uint32_t)(uintptr_t)arg4;

            if ((poke_addr & ~7) == (last_peek_addr & ~7)) {
                uint32_t old_value = (uint32_t)last_peek_value;

                if (old_value != new_value) {
                    log_ptrace_write(poke_addr, new_value);
                }
            } else {
                log_ptrace_write(poke_addr, new_value);
            }
        }
    }

    return result;
}

static uintptr_t get_module_base(const char* module_name) {
    FILE* file = fopen("/proc/self/maps", "r");
    if (!file) return 0;

    uintptr_t base = 0;
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, module_name) && strstr(line, "r-xp")) {
            sscanf(line, "%lx", &base);
            break;
        }
    }
    fclose(file);
    return base;
}

__attribute__((constructor)) static void init() {
    LOGD("Initializing GG Ptrace Hook...");

    const char* self_module = "lib5.so";
    auto base = get_module_base(self_module);
    if (!base) {
        LOGE("Failed to get module base");
        return;
    }
    LOGD("Module base: 0x%lx", base);

    for (long i = 0; ; ++i) {
        auto addr = base + i * sizeof(uintptr_t);
        if (*(uintptr_t*)addr == (uintptr_t)syscall) {
            LOGD("Found syscall at offset 0x%lx", addr - base);

            void* page_start = (void*)(addr & ~(PAGE_SIZE - 1));
            if (mprotect(page_start, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
                LOGE("mprotect failed");
                return;
            }

            *(uintptr_t*)addr = (uintptr_t)my_syscall;
            LOGD("Successfully hooked syscall!");

            break;
        }
    }
}