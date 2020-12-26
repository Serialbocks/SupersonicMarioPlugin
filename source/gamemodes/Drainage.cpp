// GameModes/Drainage.cpp
// A boost draining game mode for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.1.0 24/12/20
// BMSDK version: 95

#include "Drainage.h"


/// <summary>Renders the available options for the game mode.</summary>
void Drainage::RenderOptions()
{
    ImGui::Checkbox("Auto Deplete Boost", &autoDeplete);
    ImGui::SliderInt("Auto Deplete Boost Rate", &autoDepleteRate, 0, 100, "%d boost per second");
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool Drainage::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void Drainage::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick",
                                           [this](const ServerWrapper& caller, void* params, const std::string&) {
                                               onTick(caller, params);
                                           });
    }
    else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string Drainage::GetGameModeName()
{
    return "Drainage";
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the current server</param>
/// <param name="params">Delay since last update</param>
void Drainage::onTick(ServerWrapper server, void* params) const
{
    if (server.IsNull()) {
        ERROR_LOG("could not get the server");
        return;
    }

    // dt since last tick in seconds
    const float dt = *static_cast<float*>(params);

    ArrayWrapper<CarWrapper> cars = server.GetCars();
    for (int i = 0; i < cars.Count(); i++) {
        CarWrapper car = cars.Get(i);
        if (car.IsNull()) {
            ERROR_LOG("could not get the car");
            continue;
        }

        BoostWrapper boost = car.GetBoostComponent();
        if (boost.IsNull()) {
            ERROR_LOG("could not get the boost component");
            continue;
        }

        const float boostAmount = boost.GetCurrentBoostAmount();
        if (boostAmount > 0) {
            if (autoDeplete) {
                boost.GiveBoost2((dt * static_cast<float>(autoDepleteRate)) / 100 * -1);
            }
        }
        else {
            INFO_LOG(quote(car.GetOwnerName()) + " exploded");
            car.Demolish2(*dynamic_cast<RBActorWrapper*>(&car));
        }
    }
}
