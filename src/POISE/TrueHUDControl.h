#pragma once

namespace Loki {

    class TrueHUDControl {

    public:
        TrueHUDControl();
        static TrueHUDControl* GetSingleton();
        static float GetMaxSpecial([[maybe_unused]] RE::Actor* a_actor);
        static float GetCurrentSpecial([[maybe_unused]] RE::Actor* a_actor);

        bool TrueHUDBars;
        TRUEHUD_API::IVTrueHUD1* g_trueHUD = nullptr;
    };

};