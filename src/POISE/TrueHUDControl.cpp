#include "TrueHUDControl.h"
#include "PoiseMod.h"

Loki::TrueHUDControl::TrueHUDControl() {
    CSimpleIniA ini;
    ini.SetUnicode();
	const auto filename = L"Data/SKSE/Plugins/loki_POISE.ini";
    [[maybe_unused]] SI_Error rc = ini.LoadFile(filename);

    this->TrueHUDBars = ini.GetBoolValue("MAIN", "bTrueHUDBars", false);
}

Loki::TrueHUDControl* Loki::TrueHUDControl::GetSingleton() {
    static auto singleton = new TrueHUDControl();
    return singleton;
}

float Loki::TrueHUDControl::GetMaxSpecial([[maybe_unused]] RE::Actor* a_actor) {
	const auto ptr = PoiseMod::GetSingleton();
	const float a_result = ptr->CalculateMaxPoise(a_actor);
    return a_result;

}

float Loki::TrueHUDControl::GetCurrentSpecial([[maybe_unused]] RE::Actor* a_actor) {
    return static_cast<float>(a_actor->GetActorRuntimeData().pad0EC);
}