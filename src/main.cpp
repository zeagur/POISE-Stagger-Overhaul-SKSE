#include "POISE/PoiseMod.h"

namespace
{
	auto* plugin = SKSE::PluginDeclaration::GetSingleton();
	auto name = plugin->GetName();
	auto version = plugin->GetVersion();

	void MessageHandler(SKSE::MessagingInterface::Message* message)
	{
		switch (message->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			{
				if (const auto ptr = Loki::TrueHUDControl::GetSingleton(); ptr->TrueHUDBars) {
					if (ptr->g_trueHUD) {
						if (ptr->g_trueHUD->RequestSpecialResourceBarsControl(SKSE::GetPluginHandle()) == TRUEHUD_API::APIResult::OK) {
							ptr->g_trueHUD->RegisterSpecialResourceFunctions(
								SKSE::GetPluginHandle(),
								Loki::TrueHUDControl::GetCurrentSpecial,
								Loki::TrueHUDControl::GetMaxSpecial,
								true);
						}
					}
				}
				break;
			}
		case SKSE::MessagingInterface::kNewGame:
			{
				ActorCache::GetSingleton()->Revert();
			}
		case SKSE::MessagingInterface::kPostLoadGame:
			{
				ActorCache::GetSingleton()->Revert();
				break;
			}
		case SKSE::MessagingInterface::kPostLoad:
			{
				Loki::TrueHUDControl::GetSingleton()->g_trueHUD = static_cast<TRUEHUD_API::IVTrueHUD4*>(
					RequestPluginAPI(TRUEHUD_API::InterfaceVersion::V4));
				if (Loki::TrueHUDControl::GetSingleton()->g_trueHUD) {
					logger::info("Obtained TrueHUD API -> {0:x}", reinterpret_cast<uintptr_t>(
						Loki::TrueHUDControl::GetSingleton()->g_trueHUD));
				} else {
					logger::warn("Failed to obtain TrueHUD API");
				}
				break;
			}
		case SKSE::MessagingInterface::kPostPostLoad:
			{
			}
		default:
			break;
		}
	}
}

namespace PoiseMod {  // Papyrus Functions

    inline auto DamagePoise(RE::StaticFunctionTag*, RE::Actor* a_actor, float a_amount) -> void {

        if (!a_actor) {
			return;
		}
		auto poise = std::atomic_ref(a_actor->GetActorRuntimeData().pad0EC).fetch_sub(static_cast<int>(a_amount));
		if (poise > 100000) {
			logger::info("DamagePoise strange poise value {}", poise);
			a_actor->GetActorRuntimeData().pad0EC = 0;
		}
		if (Loki::TrueHUDControl::GetSingleton()->g_trueHUD) {
			Loki::TrueHUDControl::GetCurrentSpecial(a_actor);
		}
	}

    inline auto RestorePoise(RE::StaticFunctionTag*, RE::Actor* a_actor, const float a_amount) -> void {

        if (!a_actor) {
			return;
		}
		auto poise = std::atomic_ref(a_actor->GetActorRuntimeData().pad0EC).fetch_add(static_cast<int>(a_amount));
		if (poise > 100000) {
			logger::info("RestorePoise strange poise value {}", poise);
			a_actor->GetActorRuntimeData().pad0EC = 0;
		}
		if (Loki::TrueHUDControl::GetSingleton()->g_trueHUD) {
			Loki::TrueHUDControl::GetCurrentSpecial(a_actor);
		}
	}

    inline auto GetPoise(RE::StaticFunctionTag*, RE::Actor* a_actor) -> float {

        if (!a_actor) {
			return -1.00f;
		}
		return static_cast<float>(a_actor->GetActorRuntimeData().pad0EC);
	}

    inline auto GetMaxPoise(RE::StaticFunctionTag*, RE::Actor* a_actor) -> float {

        if (!a_actor) {
			return -1.00f;
		}
		const auto ptr = Loki::PoiseMod::GetSingleton();

		//TODO Conner: new formula; add loki's old formula back as toggleable option.
		//staggerlock is a skill issue.
		float level = a_actor->GetLevel();
		const float levelweight = ptr->MaxPoiseLevelWeight;
		const float levelweightplayer = ptr->MaxPoiseLevelWeightPlayer;
		float ArmorRating = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kDamageResist);
		const float ArmorWeight = ptr->ArmorLogarithmSlope;
		const float ArmorWeightPlayer = ptr->ArmorLogarithmSlopePlayer;
		const float RealWeight = ActorCache::GetSingleton()->GetOrCreateCachedWeight(a_actor);

		level = (level < 100 ? level : 100);
		ArmorRating = (ArmorRating > 0 ? ArmorRating : 0);
		float a_result = (RealWeight + (levelweight * level) + (a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.2f)) * (1 + log10(ArmorRating / ArmorWeight + 1));
		if (a_actor->IsPlayerRef()) {
			level = (level < 60 ? level : 60);
			a_result = (RealWeight + (levelweightplayer * level) + (a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.2f)) * (1 + log10(ArmorRating / ArmorWeightPlayer + 1));
		}

		//KFC Original Recipe.
		if (ptr->UseOldFormula) {
			a_result = (RealWeight + (a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.20f));
		}

		if (a_actor && (a_actor->GetRace()->HasKeywordString("ActorTypeCreature") || a_actor->GetRace()->HasKeywordString("ActorTypeDwarven"))) {
			for (const auto& [fst, snd] : Loki::PoiseMod::poiseRaceMap) {
				if (a_actor->GetName()) {
					const RE::TESRace* a_actorRace = a_actor->GetRace();
					const RE::TESRace* a_mapRace = fst;
					if (a_actorRace && a_mapRace) {
						if (a_actorRace->formID == a_mapRace->formID) {
							const float CreatureMult = ptr->CreatureHPMultiplier;
							a_result = CreatureMult * snd[0];
							break;
						}
					}
				}
			}
		}


		const auto effect = a_actor->GetMagicTarget()->GetActiveEffectList();
		for (const auto& veffect : *effect) {
			if (!veffect) {
				continue;
			}
			if (!veffect->GetBaseObject()) {
				continue;
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseHealthFlat")) {
				const auto buffFlat = (veffect->magnitude);
				a_result += buffFlat;  //TODO Conner: additive instead of multiplicative buff.
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("MagicArmorSpell")) {
				const auto armorflesh = (0.1f * veffect->magnitude);
				a_result += armorflesh;
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseIncrease")) {
				const auto buffPercent = (veffect->magnitude / 100.00f);	// convert to percentage
				const auto resultingBuff = (a_result * buffPercent);
				a_result += resultingBuff;	// victim has poise hp buff
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseDecrease")) {
				const auto nerfPercent = (veffect->magnitude / 100.00f);	// convert to percentage
				const auto resultingNerf = (a_result * nerfPercent);
				a_result -= resultingNerf;	// victim poise hp nerf
			}
		}


		return a_result;
	}

    inline auto SetPoise(RE::StaticFunctionTag*, RE::Actor* a_actor, float a_amount) -> void {

        if (!a_actor) {
			return;
		}
		if (a_amount > 100000) {
			logger::info("SetPoise strange poise value {}", a_amount);
			a_amount = 0;
		}
		a_actor->GetActorRuntimeData().pad0EC = static_cast<int>(a_amount);
	}

    bool RegisterFuncsForSKSE(RE::BSScript::IVirtualMachine* a_vm) {

        if (!a_vm) {
            return false;
        }

        a_vm->RegisterFunction("DamagePoise", "Loki_PoiseMod", DamagePoise, false);
        a_vm->RegisterFunction("RestorePoise", "Loki_PoiseMod", RestorePoise, false);
        a_vm->RegisterFunction("GetPoise", "Loki_PoiseMod", GetPoise, false);
        a_vm->RegisterFunction("SetPoise", "Loki_PoiseMod", SetPoise, false);

    	logger::info("Registered Poise Functionality to Papyrus interface");

        return true;

    }

}

SKSEPluginLoad(const SKSE::LoadInterface *a_skse)
{
	InitializeLog();
	logger::info(FMT_STRING("{} v{}"), name, version);
	logger::info("POISE loaded");

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(64);

	const auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {  // add callbacks for TrueHUD
		logger::info("Failed to Register TrueHUD SKSE interface");
		return false;
	}

	const auto papyrus = SKSE::GetPapyrusInterface();
	if (!papyrus->Register(PoiseMod::RegisterFuncsForSKSE)) {  // register papyrus functions
		logger::info("Failed to Register Poise Functionality to Papyrus interface");
		return false;
	}

	logger::info("Attempting to install Poise hook interfaces");
    Loki::PoiseMod::InstallStaggerHook();
    // Loki::PoiseMod::InstallWaterHook();
    Loki::PoiseMod::InstallIsActorKnockdownHook();
   // Loki::PoiseMod::InstallMagicEventSink();
	Loki::PoiseMagicDamage::InstallNFACMagicHook();
	ActorCache::RegisterEvents();

	logger::info("Finished install Poise hook interfaces");
    return true;
}