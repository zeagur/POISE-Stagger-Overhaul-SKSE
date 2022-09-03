#include "Loki_PluginTools.h"
#include "POISE/PoiseMod.h"
#include "POISE/TrueHUDAPI.h"
#include "POISE/TrueHUDControl.h"
#include "ActorCache.h"


#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("loki_POISE");
	v.AuthorName("LokiWasHere");
	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

namespace
{
	void InitializeLog()
	{
		auto path = logger::log_directory();
		if (!path) {
			stl::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%^%l%$] %v"s);

		logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
	}

	void MessageHandler(SKSE::MessagingInterface::Message* message)
	{
		switch (message->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			{
				auto ptr = Loki::TrueHUDControl::GetSingleton();
				if (ptr->TrueHUDBars) {
					if (ptr->g_trueHUD) {
						if (ptr->g_trueHUD->RequestSpecialResourceBarsControl(SKSE::GetPluginHandle()) == TRUEHUD_API::APIResult::OK) {
							ptr->g_trueHUD->RegisterSpecialResourceFunctions(SKSE::GetPluginHandle(), Loki::TrueHUDControl::GetCurrentSpecial, Loki::TrueHUDControl::GetMaxSpecial, true);
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
				Loki::TrueHUDControl::GetSingleton()->g_trueHUD = reinterpret_cast<TRUEHUD_API::IVTrueHUD3*>(TRUEHUD_API::RequestPluginAPI(TRUEHUD_API::InterfaceVersion::V3));
				if (Loki::TrueHUDControl::GetSingleton()->g_trueHUD) {
					logger::info("Obtained TrueHUD API -> {0:x}", (uintptr_t)Loki::TrueHUDControl::GetSingleton()->g_trueHUD);
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
        } else {
            int poise = (int)a_actor->pad0EC;
            poise -= (int)a_amount;
            a_actor->pad0EC = poise;
            if (a_actor->pad0EC > 100000) { a_actor->pad0EC = 0; }
			if (Loki::TrueHUDControl::GetSingleton()->g_trueHUD) {
				Loki::TrueHUDControl::GetSingleton()->GetCurrentSpecial(a_actor);
			}
        }

    }

    inline auto RestorePoise(RE::StaticFunctionTag*, RE::Actor* a_actor, float a_amount) -> void {

        if (!a_actor) {
            return;
        } else {
            int poise = (int)a_actor->pad0EC;
            poise += (int)a_amount;
            a_actor->pad0EC = poise;
            if (a_actor->pad0EC > 100000) { a_actor->pad0EC = 0; }
			if (Loki::TrueHUDControl::GetSingleton()->g_trueHUD) {
				Loki::TrueHUDControl::GetSingleton()->GetCurrentSpecial(a_actor);
			}
        }

    }

    inline auto GetPoise(RE::StaticFunctionTag*, RE::Actor* a_actor) -> float {

        if (!a_actor) {
            return -1.00f;
        } else {
            return (float)a_actor->pad0EC;
        }

    }

    inline auto GetMaxPoise(RE::StaticFunctionTag*, RE::Actor* a_actor) -> float {

        if (!a_actor) {
            return -1.00f;
        } else {
            auto ptr = Loki::PoiseMod::GetSingleton();

			//conner: new formula; add loki's old formula back as toggleable option.
			//staggerlock is a skill issue.
			float level = a_actor->GetLevel();
			float levelweight = ptr->MaxPoiseLevelWeight;
			float levelweightplayer = ptr->MaxPoiseLevelWeightPlayer;
			float ArmorRating = a_actor->GetActorValue(RE::ActorValue::kDamageResist);
			float ArmorWeight = ptr->ArmorLogarithmSlope;
			float ArmorWeightPlayer = ptr->ArmorLogarithmSlopePlayer;
			float RealWeight = ActorCache::GetSingleton()->GetOrCreateCachedWeight(a_actor);

			level = (level < 100 ? level : 100);
			ArmorRating = (ArmorRating > 0 ? ArmorRating : 0);
			float a_result = (RealWeight + (levelweight * level) + (a_actor->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.2f)) * (1 + log10(ArmorRating / ArmorWeight + 1));
			if (a_actor->IsPlayerRef()) {
				level = (level < 60 ? level : 60);
				a_result = (RealWeight + (levelweightplayer * level) + (a_actor->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.2f)) * (1 + log10(ArmorRating / ArmorWeightPlayer + 1));
			}

			//KFC Original Recipe.
			if (ptr->UseOldFormula) {
				a_result = (RealWeight + (a_actor->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.20f));
			}

			if (a_actor && a_actor->race->HasKeywordString("ActorTypeCreature") || a_actor->race->HasKeywordString("ActorTypeDwarven")) {
				for (auto idx : ptr->poiseRaceMap) {
					if (a_actor) {
						RE::TESRace* a_actorRace = a_actor->race;
						RE::TESRace* a_mapRace = idx.first;
						if (a_actorRace && a_mapRace) {
							if (a_actorRace->formID == a_mapRace->formID) {
								float CreatureMult = ptr->CreatureHPMultiplier;
								a_result = CreatureMult * idx.second[0];
								break;
							}
						}
					}
				}
			}

            
            const auto effect = a_actor->GetActiveEffectList();
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


            return a_result;
        }

    }

    inline auto SetPoise(RE::StaticFunctionTag*, RE::Actor* a_actor, float a_amount) -> void {

        if (!a_actor) {
            return;
        } else {
            a_actor->pad0EC = (int)a_amount;
            if (a_actor->pad0EC > 100000) { a_actor->pad0EC = 0; }
        }

    }

    bool RegisterFuncsForSKSE(RE::BSScript::IVirtualMachine* a_vm) {

        if (!a_vm) {
            return false;
        }

        a_vm->RegisterFunction("DamagePoise", "Loki_PoiseMod", DamagePoise, false);
        a_vm->RegisterFunction("RestorePoise", "Loki_PoiseMod", RestorePoise, false);
        a_vm->RegisterFunction("GetPoise", "Loki_PoiseMod", GetPoise, false);
        a_vm->RegisterFunction("SetPoise", "Loki_PoiseMod", SetPoise, false);

        return true;

    }

}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	logger::info("POISE loaded");

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(64);

	const auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {  // add callbacks for TrueHUD
		return false;
	}

	const auto papyrus = SKSE::GetPapyrusInterface();
	if (!papyrus->Register(PoiseMod::RegisterFuncsForSKSE)) {  // register papyrus functions
		return false;
	}

    Loki::PoiseMod::InstallStaggerHook();
    Loki::PoiseMod::InstallWaterHook();
    Loki::PoiseMod::InstallIsActorKnockdownHook();
   // Loki::PoiseMod::InstallMagicEventSink();
	Loki::PoiseMagicDamage::InstallNFACMagicHook();
	ActorCache::RegisterEvents();

    return true;
}