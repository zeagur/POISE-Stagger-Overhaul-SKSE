#include "TrueHUDControl.h"

Loki::TrueHUDControl::TrueHUDControl() {
    CSimpleIniA ini;
    ini.SetUnicode();
    auto filename = L"Data/SKSE/Plugins/loki_POISE.ini";
    [[maybe_unused]] SI_Error rc = ini.LoadFile(filename);

    this->TrueHUDBars = ini.GetBoolValue("MAIN", "bTrueHUDBars", false);
}

Loki::TrueHUDControl* Loki::TrueHUDControl::GetSingleton() {
    static Loki::TrueHUDControl* singleton = new Loki::TrueHUDControl();
    return singleton;
}

float Loki::TrueHUDControl::GetMaxSpecial([[maybe_unused]] RE::Actor* a_actor) {

    auto ptr = Loki::PoiseMod::GetSingleton();

    //float a_result = (a_actor->equippedWeight + (a_actor->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.20f));
	float level = a_actor->GetLevel();
	level = (level < 100 ? level : 100);
	float a_result = (a_actor->equippedWeight + (0.5f * level) + (a_actor->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.2f)) * (1 + (a_actor->GetActorValue(RE::ActorValue::kDamageResist)) / 250);
	if (a_actor->IsPlayerRef()) {
		level = (level < 60 ? level : 60);
		a_result = (a_actor->equippedWeight + (0.5f * level) + (a_actor->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.2f)) * (1 + (a_actor->GetActorValue(RE::ActorValue::kDamageResist)) / 1850);
	}

    if (a_actor && a_actor->race->HasKeywordString("ActorTypeCreature") || a_actor->race->HasKeywordString("ActorTypeDwarven")) {
		for (auto idx : ptr->poiseRaceMap) {
			if (a_actor) {
				RE::TESRace* a_actorRace = a_actor->race;
				RE::TESRace* a_mapRace = idx.first;
				if (a_actorRace && a_mapRace) {
					if (a_actorRace->formID == a_mapRace->formID) {
						a_result = idx.second[1];
						break;
					}
				}
			}
		}
	}

    const auto effect = a_actor->GetActiveEffectList();
	if (effect) {
		for (const auto& veffect : *effect) {
			if (!veffect) {
				continue;
			}
			if (!veffect->GetBaseObject()) {
				continue;
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseIncrease")) {
				auto resultingBuff = (veffect->magnitude);
				a_result += resultingBuff;	//conner: i made this additive not multiplicative. Easier to work with.
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseDecrease")) {
				auto resultingNerf = (veffect->magnitude);
				a_result -= resultingNerf;	//conner: this loops through and adds every buff on actor.
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("MagicArmorSpell")) {
				auto armorflesh = (0.1f * veffect->magnitude);
				a_result += armorflesh;
			}
		}
	}
    return a_result;

}

float Loki::TrueHUDControl::GetCurrentSpecial([[maybe_unused]] RE::Actor* a_actor) {
    return (float)a_actor->pad0EC;
}