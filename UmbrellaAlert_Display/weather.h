#ifndef WEATHER_H
#define WEATHER_H

#include <M5Unified.h>
#include "config.h"

typedef struct {
    const char* en;
    const char* jp;
} WeatherMap;

const WeatherMap weather_map[] = {
    {"Thunderstorm", "雷雨"},
    {"Drizzle",      "霧雨"},
    {"Rain",         "雨"},
    {"Snow",         "雪"},
    {"Clear",        "晴れ"},
    {"Clouds",       "くもり"},
    {"Mist",         "霧"},
    {"Smoke",        "煙"},
    {"Haze",         "靄"},
    {"Dust",         "砂塵風"},
    {"Fog",          "濃い霧"},
    {"Sand",         "砂"},
    {"Ash",          "火山灰"},
    {"Squall",       "突風"},
    {"Tornado",      "竜巻"}
};

const char* translate_weather(String main) {
    int size = sizeof(weather_map) / sizeof(weather_map[0]);
    for (int i = 0; i < size; i++) {
        if (strcmp(main.c_str(), weather_map[i].en) == 0) {
            return weather_map[i].jp;
        }
    }
    return "不明";
}
#endif // WEATER_H