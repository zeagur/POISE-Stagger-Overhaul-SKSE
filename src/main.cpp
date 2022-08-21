#include "Loki_PluginTools.h"
#include "POISE/PoiseMod.h"
#include "POISE/TrueHUDAPI.h"
#include "POISE/TrueHUDControl.h"

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
		spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

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
		case SKSE::MessagingInterface::kPostLoadGame:
			{
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

			//conner: new formula; add loki's old formula back as toggleable option?
			//Caveat: NPC poise doubles with every 250 points of DamageResist. For modded NPCs with high AR value this can get very stupid. Find solution.
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
			for (const auto& veffect : *effect) {
				if (!veffect) {
					continue;
				}
				if (!veffect->GetBaseObject()) {
					continue;
				}
				if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseIncrease")) {
					auto resultingBuff = (veffect->magnitude);
					a_result += resultingBuff;	// victim has poise hp buff	
												//conner: i made this additive not multiplicative. Easier to work with.
				}
				if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseDecrease")) {
					auto resultingNerf = (veffect->magnitude);
					a_result -= resultingNerf;	// victim poise hp nerf
												//conner: this loops through and adds every buff on actor.
				}
				if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("MagicArmorSpell")) {
					auto armorflesh = (0.1f * veffect->magnitude);
					a_result += armorflesh;
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
    Loki::PoiseMod::InstallMagicEventSink();

    return true;
}