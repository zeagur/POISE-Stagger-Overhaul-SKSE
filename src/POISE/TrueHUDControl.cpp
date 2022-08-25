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

    //Scales logarithmically with armorrating.
	float level = a_actor->GetLevel();
	float levelweight = ptr->MaxPoiseLevelWeight;
	float levelweightplayer = ptr->MaxPoiseLevelWeightPlayer;
	float ArmorRating = a_actor->GetActorValue(RE::ActorValue::kDamageResist);
	float ArmorWeight = ptr->ArmorLogarithmSlope;
	float ArmorWeightPlayer = ptr->ArmorLogarithmSlopePlayer;

	level = (level < 100 ? level : 100);
	float a_result = (a_actor->equippedWeight + (levelweight * level) + (a_actor->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.2f)) * (1 + log10(ArmorRating / ArmorWeight + 1));
	if (a_actor->IsPlayerRef()) {
		level = (level < 60 ? level : 60);
		a_result = (a_actor->equippedWeight + (levelweightplayer * level) + (a_actor->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.2f)) * (1 + log10(ArmorRating / ArmorWeightPlayer + 1));
	}

	//KFC Original Recipe.
    if (ptr->UseOldFormula) {
		a_result = (a_actor->equippedWeight + (a_actor->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.20f));
	}

    if (a_actor && a_actor->race->HasKeywordString("ActorTypeCreature") || a_actor->race->HasKeywordString("ActorTypeDwarven")) {
		for (auto idx : ptr->poiseRaceMap) {
			if (a_actor) {
				RE::TESRace* a_actorRace = a_actor->race;
				RE::TESRace* a_mapRace = idx.first;
				if (a_actorRace && a_mapRace) {
					if (a_actorRace->formID == a_mapRace->formID) {
						a_result = idx.second[0];
						break;
					}
				}
			}
		}
	}

    if (ptr->ScalePHealthGlobal) {
		if (a_actor->IsPlayerRef()) {
			a_result *= ptr->GlobalPlayerPHealthMult;
		} else {
			a_result *= ptr->GlobalPHealthMult;
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
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseHealthFlat")) {
				auto buffFlat = (veffect->magnitude);
				a_result += buffFlat;  //conner: additive instead of multiplicative buff.
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("MagicArmorSpell")) {
				auto armorflesh = (0.1f * veffect->magnitude);
				a_result += armorflesh;
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseIncrease")) {
				auto buffPercent = (veffect->magnitude / 100.00f);	// convert to percentage
				auto resultingBuff = (a_result * buffPercent);
				a_result += resultingBuff;	// victim has poise hp buff
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseDecrease")) {
				auto nerfPercent = (veffect->magnitude / 100.00f);	// convert to percentage
				auto resultingNerf = (a_result * nerfPercent);
				a_result -= resultingNerf;	// victim poise hp nerf
			}
		}
	}
    return a_result;

}

float Loki::TrueHUDControl::GetCurrentSpecial([[maybe_unused]] RE::Actor* a_actor) {
    return (float)a_actor->pad0EC;
}