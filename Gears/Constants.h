#pragma once
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define VERSION_MAJOR 5
#define VERSION_MINOR 3
#define VERSION_PATCH 0

namespace Constants {
    static const char* const DisplayVersion = "v" STR(VERSION_MAJOR) "."  STR(VERSION_MINOR) "." STR(VERSION_PATCH);
    static const char* const ModDir = "\\ManualTransmission";
    static const char* const NotificationPrefix =  "~b~Manual Transmission~w~";
}
