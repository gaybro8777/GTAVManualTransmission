#include "script.h"

#include "Constants.h"
#include "GitInfo.h"

#include "SteeringAnim.h"

#include "Compatibility.h"
#include "Memory/MemoryPatcher.hpp"
#include "Memory/VehicleExtensions.hpp"
#include "Memory/Versions.h"
#include "Util/FileVersion.h"
#include "Util/Logger.hpp"
#include "Util/Paths.h"

#include <inc/main.h>
#include <Psapi.h>
#include <filesystem>

namespace fs = std::filesystem;

void resolveVersion() {
    int shvVersion = getGameVersion();

    logger.Write(INFO, "SHV API Game version: %s (%d)", eGameVersionToString(shvVersion).c_str(), shvVersion);
    // Also prints the other stuff, annoyingly.
    SVersion exeVersion = getExeInfo();

    if (shvVersion < G_VER_1_0_877_1_STEAM) {
        logger.Write(WARN, "Outdated game version! Update your game.");
    }

    // Version we *explicitly* support
    std::vector<int> exeVersionsSupp = findNextLowest(ExeVersionMap, exeVersion);
    if (exeVersionsSupp.empty() || exeVersionsSupp.size() == 1 && exeVersionsSupp[0] == -1) {
        logger.Write(ERROR, "Failed to find a corresponding game version.");
        logger.Write(WARN, "    Using SHV API version [%s] (%d)",
            eGameVersionToString(shvVersion).c_str(), shvVersion);
        MemoryPatcher::SetPatterns(shvVersion);
        return;
    }

    int highestSupportedVersion = *std::max_element(std::begin(exeVersionsSupp), std::end(exeVersionsSupp));
    if (shvVersion > highestSupportedVersion) {
        logger.Write(WARN, "Game newer than last supported version");
        logger.Write(WARN, "    You might experience instabilities or crashes");
        logger.Write(WARN, "    Using SHV API version [%s] (%d)",
            eGameVersionToString(shvVersion).c_str(), shvVersion);
        MemoryPatcher::SetPatterns(shvVersion);
        return;
    }

    int lowestSupportedVersion = *std::min_element(std::begin(exeVersionsSupp), std::end(exeVersionsSupp));
    if (shvVersion < lowestSupportedVersion) {
        logger.Write(WARN, "SHV API reported lower version than actual EXE version.");
        logger.Write(WARN, "    EXE version     [%s] (%d)",
            eGameVersionToString(lowestSupportedVersion).c_str(), lowestSupportedVersion);
        logger.Write(WARN, "    SHV API version [%s] (%d)",
            eGameVersionToString(shvVersion).c_str(), shvVersion);
        logger.Write(WARN, "    Using EXE version, or highest supported version [%s] (%d)",
            eGameVersionToString(lowestSupportedVersion).c_str(), lowestSupportedVersion);
        MemoryPatcher::SetPatterns(lowestSupportedVersion);
        // Actually need to change stuff
        VehicleExtensions::ChangeVersion(lowestSupportedVersion);
        return;
    }

    logger.Write(INFO, "Using offsets based on SHV API version [%s] (%d)",
        eGameVersionToString(shvVersion).c_str(), shvVersion);
    MemoryPatcher::SetPatterns(shvVersion);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    const std::string modPath = Paths::GetModuleFolder(hInstance) + Constants::ModDir;
    const std::string logFile = modPath + "\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";

    if (!fs::is_directory(modPath) || !fs::exists(modPath)) {
        fs::create_directory(modPath);
    }

    logger.SetFile(logFile);
    Paths::SetOurModuleHandle(hInstance);

    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            logger.Clear();
            logger.Write(INFO, "Manual Transmission %s (built %s %s) (%s)",
                Constants::DisplayVersion, __DATE__, __TIME__, GIT_HASH GIT_DIFF);

            resolveVersion();

            scriptRegister(hInstance, ScriptMain);
            scriptRegisterAdditionalThread(hInstance, NPCMain);
            
            logger.Write(INFO, "Script registered");
            break;
        }
        case DLL_PROCESS_DETACH: {
            logger.Write(INFO, "PATCH: Init shutdown");
            const uint8_t expected = 6;
            uint8_t actual = 0;

            if (MemoryPatcher::RevertGearboxPatches()) 
                actual++;
            if (MemoryPatcher::RestoreSteeringAssist())
                actual++;
            if (MemoryPatcher::RestoreSteeringControl()) 
                actual++;
            if (MemoryPatcher::RestoreBrake())
                actual++;
            if (MemoryPatcher::RestoreThrottle())
                actual++;
            if (MemoryPatcher::RestoreThrottleControl())
                actual++;

            if (actual == expected) {
                logger.Write(INFO, "PATCH: Script shut down cleanly");
            }
            else {
                logger.Write(ERROR, "PATCH: Script shut down with unrestored patches!");
            }

            resetSteeringMultiplier();
            releaseCompatibility();

            SteeringAnimation::CancelAnimation();

            scriptUnregister(hInstance);
            break;
        }
        default:
            // Do nothing
            break;
    }
    return TRUE;
}
