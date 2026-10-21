// SCDA-XBERecomp's logging, on stderr.
#pragma once
#include <cstdio>

#define LOG_FIRST(n, ...)                                       \
    do {                                                        \
        static int log_first_count_ = 0;                        \
        if (log_first_count_ < (n)) {                           \
            log_first_count_++;                                 \
            std::fprintf(stderr, "[D3D] " __VA_ARGS__);         \
            std::fputc('\n', stderr);                           \
        }                                                       \
    } while (0)

#define LOGI(...) do { std::fprintf(stderr, "[D3D] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#define LOGE(...) do { std::fprintf(stderr, "[D3D ERROR] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#define LOGR(...) do { std::fprintf(stderr, "[D3D] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)

