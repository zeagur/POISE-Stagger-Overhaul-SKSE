#pragma once

#include <shared_mutex>
#include "Loki_PluginTools.h"
#include "TrueHUDControl.h"

namespace Loki {

    class PoiseMagicDamage
	{
    public:
        static PoiseMagicDamage* GetSingleton();
		static void PoiseCalculateMagic(RE::MagicCaster* a_magicCaster, RE::Projectile* a_projectile, RE::TESObjectREFR* a_target);

        //auto ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource)->RE::BSEventNotifyControl override;

		// This hook includes magic projectile from spells and enchanted arrows
		struct MagicProjectileHitHook
		{
			static void thunk(RE::MagicCaster* a_magicCaster, void* a_unk1, RE::Projectile* a_projectile, RE::TESObjectREFR* a_target, float a_unk2, float a_unk3)
			{
				//auto attacker = a_magicCaster->GetCaster();

				func(a_magicCaster, a_unk1, a_projectile, a_target, a_unk2, a_unk3);
				GetSingleton()->PoiseCalculateMagic(a_magicCaster, a_projectile, a_target);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

        static void InstallNFACMagicHook()
		{
#ifdef SKYRIM_AE
			stl::write_thunk_call<MagicProjectileHitHook>(REL::ID(44206).address() + 0x218);
#else
			stl::write_thunk_call<MagicProjectileHitHook>(REL::ID(43015).address() + 0x216);
#endif
		}

    protected:
        PoiseMagicDamage() = default;
        PoiseMagicDamage(const PoiseMagicDamage&) = delete;
        PoiseMagicDamage(PoiseMagicDamage&&) = delete;
        virtual ~PoiseMagicDamage() = default;

        auto operator=(const PoiseMagicDamage&)->PoiseMagicDamage & = delete;
        auto operator=(PoiseMagicDamage&&)->PoiseMagicDamage & = delete;
    };

    class PoiseMod {

    public:
        bool ConsoleInfoDump;

        float poiseBreakThreshhold0, poiseBreakThreshhold1, poiseBreakThreshhold2;
        float BowMult, CrossbowMult, Hand2Hand, OneHandAxe, OneHandDagger, OneHandMace, OneHandSword, TwoHandAxe, TwoHandSword;
        float RapierMult, PikeMult, SpearMult, HalberdMult, QtrStaffMult, CaestusMult, ClawMult, WhipMult;
		float PowerAttackMult, BlockedMult, BashMult, HyperArmourMult, NPCHyperArmourMult, SpellHyperArmourMult, NPCSpellHyperArmourMult;
		float BashParryMult, GlobalPHealthMult, GlobalPDmgMult, GlobalPlayerPHealthMult, GlobalPlayerPDmgMult;
		float MaxPoiseLevelWeight, MaxPoiseLevelWeightPlayer, PhysicalDmgWeight, PhysicalDmgWeightPlayer, SpellPoiseEffectWeight, SpellPoiseConcMult;
		float ArmorLogarithmSlope, ArmorLogarithmSlopePlayer, WardPowerWeight, CreatureHPMultiplier, CreatureDMGMultiplier, HyperArmorLogSlope, SpellHyperLogSlope; 
        bool PlayerPoiseEnabled, NPCPoiseEnabled, PlayerRagdollReplacer, NPCRagdollReplacer, PoiseRegenEnabled, TrueHUDBars;
		bool ScalePHealthGlobal, ScalePDmgGlobal, SpellPoise, PlayerSpellPoise, ForceThirdPerson, UseOldFormula; 

        const RE::BSFixedString ae_Stagger = "staggerStart";
        const RE::BSFixedString staggerDire = "staggerDirection";
        const RE::BSFixedString staggerMagn = "staggerMagnitude";
        const RE::BSFixedString isBlocking = "IsBlocking";
        const RE::BSFixedString isAttacking = "IsAttacking";
        const RE::BSFixedString setStaggerDire = "setstagger";

        const RE::BSFixedString poiseSmallBwd = "poise_small_start";
        const RE::BSFixedString poiseSmallFwd = "poise_small_start_fwd";

        const RE::BSFixedString poiseMedBwd = "poise_med_start";
        const RE::BSFixedString poiseMedFwd = "poise_med_start_fwd";

        const RE::BSFixedString poiseLargeBwd = "poise_large_start";
        const RE::BSFixedString poiseLargeFwd = "poise_large_start_fwd";

        const RE::BSFixedString poiseLargestBwd = "poise_largest_start";
        const RE::BSFixedString poiseLargestFwd = "poise_largest_start_fwd";

        RE::SpellItem* poiseDelaySpell = NULL;
        RE::EffectSetting* poiseDelayEffect = NULL;
        RE::BGSKeyword* kCreature = NULL;
        RE::BGSKeyword* kDragon = NULL;
        RE::BGSKeyword* kGiant = NULL;
        RE::BGSKeyword* kGhost = NULL;
        RE::BGSKeyword* kDwarven = NULL;
        RE::BGSKeyword* kTroll = NULL;
        RE::BGSKeyword* WeapMaterialSilver = NULL;

        RE::EffectSetting* HardcodeFus1 = NULL;
		RE::EffectSetting* HardcodeFus2 = NULL;
        //hardcode fus only temporarily, next update switch to ini table of MagicEffects to stagger.
 
 
        //RE::BGSKeyword* PoiseDmgNerf = NULL;
        //RE::BGSKeyword* PoiseDmgBuff = NULL;
        //RE::BGSKeyword* PoiseHPNerf = NULL;
        //RE::BGSKeyword* PoiseHPBuff = NULL;

        static inline std::unordered_map<RE::TESRace*, std::vector<float>> poiseRaceMap;

        PoiseMod();
        static void ReadPoiseTOML();
        static PoiseMod* GetSingleton();

        static void InstallStaggerHook();
        static void InstallWaterHook();
        static void InstallIsActorKnockdownHook();
        //static void InstallMagicEventSink();

        static float CalculatePoiseDamage(RE::HitData& a_hitData, RE::Actor* a_actor);
        static float CalculateMaxPoise(RE::Actor* a_actor);


    private:
        static bool IsActorKnockdown(RE::Character* a_this, std::int64_t a_unk);
        static float GetSubmergedLevel(RE::Actor* a_actor, float a_zPos, RE::TESObjectCELL* a_cell);
        static void ProcessHitEvent(RE::Actor* a_actor, RE::HitData& a_hitData);

        static inline REL::Relocation<decltype(GetSubmergedLevel)> _GetSubmergedLevel;
        static inline REL::Relocation<decltype(ProcessHitEvent)> _ProcessHitEvent;
        static inline REL::Relocation<decltype(IsActorKnockdown)> _IsActorKnockdown;

    protected:

    };

};