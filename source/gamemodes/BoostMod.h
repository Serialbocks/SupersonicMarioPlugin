#pragma once
#include "../RocketPlugin.h"


class BoostMod final : public RocketGameMode
{
public:
    explicit BoostMod(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

    struct BoostModifier {
        // Whether the modifier should be considered.
        bool Enabled = false;
        // Maximum amount of boost player can have. (Range: 0-100)
        float MaxBoost = 100.f;
        // Boost amount modifiers.
        std::vector<std::string> BoostAmountModifiers = {
            "Default", "No boost", "Unlimited", "Recharge (slow)", "Recharge (fast)"
        };
        std::size_t BoostAmountModifier = 0;
    };

private:
    void onTick(ServerWrapper server);

    BoostModifier boostModifierGeneral;
    std::array<BoostModifier, 2> boostModifierTeams;
};
