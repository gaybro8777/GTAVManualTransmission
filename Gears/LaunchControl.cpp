#include "LaunchControl.h"

#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Input/CarControls.hpp"
#include "Util/MathExt.h"
#include "Util/UIUtils.h"

#include <inc/enums.h>
#include <inc/natives.h>

extern CarControls g_controls;
extern ScriptSettings g_settings;
extern Vehicle g_playerVehicle;
extern VehicleData g_vehData;

namespace {
    LaunchControl::ELCState launchState = LaunchControl::ELCState::Inactive;
}

void LaunchControl::Update(float& clutchVal) {
    if (g_settings().DriveAssists.LaunchControl.Enable &&
        g_vehData.mGearCurr == 1 &&
        VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle)) {
        switch (launchState) {
            case ELCState::Inactive: {
                //(g_gearStates.FakeNeutral || clutch >= 1.0f || VExt::GetHandbrake(g_playerVehicle)) && )
                if (Math::Near(Length(g_vehData.mVelocity), 0.0f, 0.1f) &&
                    g_controls.BrakeVal > 0.1f) {
                    launchState = ELCState::Staged;
                }
                break;
            }
            case ELCState::Staged: {
                if (g_controls.BrakeVal == 0.0f && g_controls.ThrottleVal == 0.0f ||
                    !Math::Near(Length(g_vehData.mVelocity), 0.0f, 0.1f)) {
                    launchState = ELCState::Inactive;
                }
                else if (g_controls.BrakeVal > 0.1f && g_controls.ThrottleVal > 0.1f) {
                    launchState = ELCState::Limiting;
                    PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
                }
                break;
            }
            case ELCState::Limiting: {
                if (g_controls.BrakeVal == 0.0f && g_controls.ThrottleVal == 0.0f) {
                    launchState = ELCState::Inactive;
                }
                else if (!Math::Near(Length(g_vehData.mVelocity), 0.0f, 0.1f)) {
                    launchState = ELCState::Controlling;
                }
                else if (g_controls.BrakeVal > 0.1f && g_controls.ThrottleVal > 0.1f) {
                    PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
                    VEHICLE::SET_VEHICLE_BURNOUT(g_playerVehicle, false);
                    clutchVal = -5.0f;
                    if (g_vehData.mRPM > g_settings().DriveAssists.LaunchControl.RPM) {
                        PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
                    }
                }
                break;
            }
            case ELCState::Controlling: {
                if (g_controls.BrakeVal == 0.0f && g_controls.ThrottleVal == 0.0f) {
                    launchState = ELCState::Inactive;
                }
                // Launch control / traction control logic happens in the assist/patchy code
                break;
            }
            default:
                break;
        }
    }
    else {
        launchState = ELCState::Inactive;
    }
}

LaunchControl::ELCState LaunchControl::GetState() {
    return launchState;
}
