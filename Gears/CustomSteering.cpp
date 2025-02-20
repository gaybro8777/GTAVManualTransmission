#include "CustomSteering.h"

#include "SteeringAnim.h"
#include "ScriptSettings.hpp"
#include "Util/MathExt.h"
#include "Util/UIUtils.h"
#include "Util/ScriptUtils.h"
#include "Memory/VehicleExtensions.hpp"
#include "Memory/VehicleBone.h"

#include <inc/enums.h>
#include <inc/natives.h>
#include <inc/types.h>

#include <algorithm>

using VExt = VehicleExtensions;

extern Vehicle g_playerVehicle;
extern Ped g_playerPed;
extern ScriptSettings g_settings;
extern CarControls g_controls;

namespace {
    float steerPrev = 0.0f;
    long long lastTickTime = 0;

    bool mouseDown = false;
    float mouseXTravel = 0.0f;

    float lastReduction = 1.0f;
}

namespace CustomSteering {
    float calculateReduction();
    float calculateDesiredHeading(float steeringMax, float desiredHeading, float reduction);

    // Doesn't change steer when not mouse steering
    bool updateMouseSteer(float& steer);
}

float CustomSteering::GetMouseX() {
    return mouseXTravel;
}

float CustomSteering::calculateReduction() {
    Vehicle mVehicle = g_playerVehicle;
    float mult = 1;
    Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(mVehicle);
    Vector3 pos = ENTITY::GET_ENTITY_COORDS(mVehicle, 1);
    Vector3 motion = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(mVehicle, pos.x + vel.x, pos.y + vel.y,
        pos.z + vel.z);
    if (motion.y > 3) {
        mult = 0.15f + powf(0.9f, abs(motion.y) - 7.2f);
        if (mult != 0) { mult = floorf(mult * 1000) / 1000; }
        if (mult > 1) { mult = 1; }
    }
    mult = (1 + (mult - 1) * g_settings.CustomSteering.SteeringReduction);
    return mult;
}

// Returns in radians
float CustomSteering::calculateDesiredHeading(float steeringMax, float desiredHeading, float reduction) {
    // Scale input with both reduction and steering limit
    float correction;

    Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true);
    if (abs(speedVector.y) > 3.0f) {
        Vector3 target = Normalize(speedVector);
        float travelDir = atan2(target.y, target.x) - static_cast<float>(M_PI) / 2.0f;
        if (travelDir > static_cast<float>(M_PI) / 2.0f) {
            travelDir -= static_cast<float>(M_PI);
        }
        if (travelDir < -static_cast<float>(M_PI) / 2.0f) {
            travelDir += static_cast<float>(M_PI);
        }
        // Correct for reverse
        travelDir *= sgn(speedVector.y);

        // Custom user multiplier. Division by steeringmult needed to neutralize over-correction.
        travelDir *= g_settings.CustomSteering.CountersteerMult / g_settings().Steering.CustomSteering.SteeringMult;

        // clamp auto correction to countersteer limit
        travelDir = std::clamp(travelDir, deg2rad(-g_settings.CustomSteering.CountersteerLimit), deg2rad(g_settings.CustomSteering.CountersteerLimit));

        float finalReduction = map(abs(travelDir), 0.0f, g_settings.CustomSteering.CountersteerLimit, reduction, 1.0f);

        // CountersteerLimit can be 0, resulting in finalReduction == nan.
        if (std::isnan(finalReduction)) {
            finalReduction = reduction;
        }

        correction = travelDir + desiredHeading * finalReduction * steeringMax;
    }
    else {
        correction = desiredHeading * reduction * steeringMax;
    }
    return std::clamp(correction, -steeringMax, steeringMax);
}

void CustomSteering::DrawDebug() {
    float steeringAngle = VExt::GetSteeringAngle(g_playerVehicle);

    Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true);
    Vector3 positionWorld = ENTITY::GET_ENTITY_COORDS(g_playerVehicle, 1);
    Vector3 travelRelative = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, speedVector.x, speedVector.y, speedVector.z);

    float steeringAngleRelX = ENTITY::GET_ENTITY_SPEED(g_playerVehicle) * -sin(steeringAngle);
    float steeringAngleRelY = ENTITY::GET_ENTITY_SPEED(g_playerVehicle) * cos(steeringAngle);
    Vector3 steeringWorld = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, steeringAngleRelX, steeringAngleRelY, 0.0f);

    //showText(0.3f, 0.15f, 0.5f, fmt::format("Angle: {}", rad2deg(steeringAngle)));

    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, travelRelative.x, travelRelative.y, travelRelative.z, 0, 255, 0, 255);
    UI::DrawSphere(travelRelative, 0.25f, { 0, 255, 0, 255 });
    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, steeringWorld.x, steeringWorld.y, steeringWorld.z, 255, 0, 255, 255);
    UI::DrawSphere(steeringWorld, 0.25f, { 255, 0, 255, 255 });
}

// Only disable the "magic" movements, such as burnout or in-air acrobatics.
// Exceptions:
//   - Bikes
//      - Should be able to pitch.
//      - Should be able to shift weight.
//          - Needs testing with default handling?
//   - Special functions
//      - Tow arm
//      - Forklift
//   - Amphibious when wet
//   - Flying when in-air
void disableControls() {
    Hash vehicleModel = ENTITY::GET_ENTITY_MODEL(g_playerVehicle);

    bool allWheelsOnGround = VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(g_playerVehicle);
    float submergeLevel = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(g_playerVehicle);
    // 5: Stromberg
    // 6: Amphibious cars / APC
    // 7: Amphibious bike
    int modelType = VExt::GetModelType(g_playerVehicle);
    bool isFrog = modelType == 5 || modelType == 6 || modelType == 7;//*(int*)(modelInfo + 0x340) == 6 || *(int*)(modelInfo + 0x340) == 7;
    bool hasFork = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(g_playerVehicle, "forks") != -1;
    bool hasTow = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(g_playerVehicle, "tow_arm") != -1;
    bool hasScoop = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(g_playerVehicle, "scoop") != -1;
    bool hasFrame1 = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(g_playerVehicle, "frame_1") != -1;
    bool hasFrame2 = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(g_playerVehicle, "frame_2") != -1;
    bool hasFrame = hasFrame1 && hasFrame2;

    bool hasEquipment = hasFork || hasTow || hasScoop || hasFrame;

    float hoverRatio = VExt::GetHoverTransformRatio(g_playerVehicle);

    bool disableLeftRight = false;
    bool disableUpDown = false;

    // Never disable anything for bikes
    if (VEHICLE::IS_THIS_MODEL_A_BIKE(vehicleModel)) {
        disableUpDown = false;
        disableLeftRight = false;
    }
    // Don't disable anything for wet amphibious vehicles
    else if (isFrog && submergeLevel > 0.0f) {
        disableUpDown = false;
        disableLeftRight = false;
    }
    // Don't disable when hovering
    else if (hoverRatio > 0.0f) {
        disableUpDown = false;
        disableLeftRight = false;
    }
    // Don't disable up/down in forklift and towtrucks
    else if (hasEquipment && allWheelsOnGround) {
        disableUpDown = false;
        disableLeftRight = true;
    }
    // Everything else
    else {
        disableLeftRight = true;
        disableUpDown = true;
    }

    if (disableUpDown)
        PAD::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveUpDown, true);
    if (disableLeftRight)
        PAD::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveLeftRight, true);
}

// Main custom steering function - called by main loop.
void CustomSteering::Update() {
    if (!Util::VehicleAvailable(g_playerVehicle, g_playerPed))
        return;

    float limitRadians = VExt::GetMaxSteeringAngle(g_playerVehicle);
    float reduction = calculateReduction();

    float steer = -PAD::GET_DISABLED_CONTROL_NORMAL(1, ControlMoveLeftRight);

    if (!g_settings.Controller.Native.Enable &&
        g_settings.Controller.CustomDeadzone &&
        g_controls.PrevInput == CarControls::Controller) {
        steer = g_controls.SteerVal;
    }

    float steerCurr;

    float steerValGammaL = pow(-steer, g_settings.CustomSteering.Gamma);
    float steerValGammaR = pow(steer, g_settings.CustomSteering.Gamma);
    float steerValGamma = steer < 0.0f ? -steerValGammaL : steerValGammaR;

    float secondsSinceLastTick = static_cast<float>(GetTickCount64() - lastTickTime) / 1000.0f;

    if (steer == 0.0f) {
        steerCurr = lerp(
            steerPrev,
            steerValGamma,
            1.0f - pow(g_settings.CustomSteering.CenterTime, secondsSinceLastTick));
    }
    else {
        steerCurr = lerp(
            steerPrev,
            steerValGamma,
            1.0f - pow(g_settings.CustomSteering.SteerTime, secondsSinceLastTick));
    }

    bool mouseInputThisTick = false;
    // Mouse input inherits lerp-to-center
    if (g_settings.CustomSteering.Mouse.Enable)
        mouseInputThisTick = updateMouseSteer(steerCurr);

    lastTickTime = GetTickCount64();
    steerPrev = steerCurr;

    // Ignore reduction for wet vehicles.
    int modelType = VExt::GetModelType(g_playerVehicle);
    bool isFrog = modelType == 5 || modelType == 6 || modelType == 7;
    bool isBoat = modelType == 13 || modelType == 15;
    float submergeLevel = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(g_playerVehicle);
    bool is2Wheel = modelType == 11 || modelType == 12 || modelType == 3 || modelType == 7;

    if (isFrog && submergeLevel > 0.0f || isBoat || is2Wheel)
        reduction = 1.0f;

    // Ignore reduction on handbrake.
    if (g_settings.CustomSteering.NoReductionHandbrake && VExt::GetHandbrake(g_playerVehicle)) {
        reduction = 1.0f;
    }

    float desiredHeading;
    if (mouseInputThisTick && g_settings.CustomSteering.Mouse.DisableReduction) {
        reduction = 1.0f;
    }
    reduction = lerp(
        lastReduction,
        reduction,
        1.0f - pow(g_settings.CustomSteering.CenterTime, secondsSinceLastTick));
    lastReduction = reduction;

    if (mouseInputThisTick && g_settings.CustomSteering.Mouse.DisableSteerAssist) {
        desiredHeading = steerCurr * limitRadians * reduction;
    }
    else {
        desiredHeading = calculateDesiredHeading(limitRadians, steerCurr, reduction);
    }

    disableControls();

    VExt::SetSteeringInputAngle(g_playerVehicle, desiredHeading * (1.0f / limitRadians));

    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle))
        VExt::SetSteeringAngle(g_playerVehicle, desiredHeading);

    auto boneIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(g_playerVehicle, "steeringwheel");
    if (boneIdx != -1 && g_settings().Steering.CustomSteering.UseCustomLock) {
        Vector3 rotAxis{};
        rotAxis.y = 1.0f;

        float corrDesiredHeading = -VExt::GetSteeringAngle(g_playerVehicle) * (1.0f / limitRadians);
        float rotRad = deg2rad(g_settings().Steering.CustomSteering.SoftLock) / 2.0f * corrDesiredHeading;
        float rotRadRaw = rotRad;

        // Setting angle using the VExt:: calls above causes the angle to overshoot the "real" coords
        // Not sure if this is the best solution, but hey, it works!
        rotRad -= 2.0f * (corrDesiredHeading * VExt::GetMaxSteeringAngle(g_playerVehicle));

        Vector3 scale { 1.0f, 0, 1.0f, 0, 1.0f, 0 };
        if (g_settings.Misc.HideWheelInFPV && CAM::GET_FOLLOW_PED_CAM_VIEW_MODE() == 4) {
            scale.x = 0.0f;
            scale.y = 0.0f;
            scale.z = 0.0f;
        }

        VehicleBones::RotateAxis(g_playerVehicle, boneIdx, rotAxis, rotRad);
        VehicleBones::Scale(g_playerVehicle, boneIdx, scale);
        SteeringAnimation::SetRotation(rotRadRaw);
    }
}

bool CustomSteering::updateMouseSteer(float& steer) {
    bool mouseControl =
        PAD::GET_CONTROL_NORMAL(0, eControl::ControlVehicleMouseControlOverride) != 0.0f &&
        !PLAYER::IS_PLAYER_FREE_AIMING(PLAYER::GET_PLAYER_INDEX());

    // on press
    if (!mouseDown && mouseControl) {
        mouseDown = true;
        PLAYER::SET_PLAYER_CAN_DO_DRIVE_BY(PLAYER::GET_PLAYER_INDEX(), false);
    }

    // on release
    if (mouseDown && !mouseControl) {
        mouseDown = false;
        PLAYER::SET_PLAYER_CAN_DO_DRIVE_BY(PLAYER::GET_PLAYER_INDEX(), true);
    }

    if (mouseDown && mouseControl) {
        PAD::DISABLE_CONTROL_ACTION(0, eControl::ControlLookLeftRight, true);
        PAD::DISABLE_CONTROL_ACTION(0, eControl::ControlLookUpDown, true);

        float mouseVal = PAD::GET_DISABLED_CONTROL_NORMAL(0, eControl::ControlLookLeftRight);
        mouseXTravel += mouseVal * g_settings.CustomSteering.Mouse.Sensitivity;
        mouseXTravel = std::clamp(mouseXTravel, -1.0f, 1.0f);

        steer = -mouseXTravel;
        return true;
    }
    else {
        // Inherit value when not controlling
        mouseXTravel = -steer;
        return false;
    }
}
