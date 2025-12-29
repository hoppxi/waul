#include "config.hpp"
#include "common.hpp"
#include <cstdio>
#include <cstring>
#include <sstream>

namespace waul {

ConfigState Config::state;

ConfigState& Config::get() { return state; }

std::string Config::get_path() {
    return get_config_dir() + "/config.ini";
}

static void parse_ints(char* str, int* out, int max) {
    int count = 0;
    char* token = strtok(str, " \t\n");
    while (token && count < max) {
        out[count++] = atoi(token);
        token = strtok(nullptr, " \t\n");
    }
    if (count == 1) {
        for(int i=1; i<max; i++) out[i] = out[0];
    }
}

void Config::load() {
    state = ConfigState();
    
    FILE* f = fopen(get_path().c_str(), "r");
    if (!f) {
        log_msg(WARN, "Config file not found at %s", get_path().c_str());
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char* start = line;
        while (*start == ' ' || *start == '\t') start++;
        if (*start == '#' || *start == ';' || *start == '\n' || *start == 0) continue;

        char* eq = strchr(start, '=');
        if (!eq) continue;
        *eq = 0;
        
        char* key = start;
        char* val = eq + 1;

        char* k_end = key + strlen(key) - 1;
        while (k_end > key && (*k_end == ' ' || *k_end == '\t')) *k_end-- = 0;

        if (strcmp(key, "margin") == 0) parse_ints(val, state.m, 4);
        else if (strcmp(key, "border_width") == 0) parse_ints(val, state.bw, 4);
        else if (strcmp(key, "border_radius") == 0) parse_ints(val, state.br, 4);
        else if (strcmp(key, "border_color") == 0) parse_ints(val, state.bc, 4);
        else if (strcmp(key, "background_color") == 0) parse_ints(val, state.bg, 4);
    }
    fclose(f);
    log_msg(INFO, "Config loaded");
}

}