#pragma once

#include <time.h>
#include <stdio.h>
#include <sys/time.h>

// #define DISABLE_DEBUG
// #define DISABLE_INFO

#ifdef DISABLE_DEBUG
    #define DEBUG(fmt, ...) (0)
#else
#define DEBUG(msg, ...) { \
  struct timeval tv; \
  gettimeofday(&tv, NULL); \
  struct tm *tm_info = localtime(&tv.tv_sec); \
  printf("[%02d:%02d:%02d.%03ld] debug :: ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, tv.tv_usec / 1000); \
  printf((msg), ##__VA_ARGS__); \
}
#endif

#ifdef DISABLE_INFO
    #define INFO(fmt, ...) (0)
#else
#define INFO(msg, ...) { \
  struct timeval tv; \
  gettimeofday(&tv, NULL); \
  struct tm *tm_info = localtime(&tv.tv_sec); \
  printf("[%02d:%02d:%02d.%03ld] info  :: ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, tv.tv_usec / 1000); \
  printf((msg), ##__VA_ARGS__); \
}
#endif
