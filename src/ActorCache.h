#pragma once

class TESFormDeleteEventHandler : public RE::BSTEventSink<RE::TESFormDeleteEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>* a_eventSource) override;
	static bool                      Register();
};

class TESEquipEventEventHandler : public RE::BSTEventSink<RE::TESEquipEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override;
	static bool                      Register();
};

class ActorCache
{
public:

	static ActorCache* GetSingleton()
	{
		static ActorCache cache;
		return &cache;
	}

	struct ActorData
	{
		float trueWeightValue;
	};

	std::shared_mutex formCacheLock;
	std::unordered_map<RE::FormID, ActorData> formCache;

	void FormDelete(RE::FormID a_formID);
	void EquipEvent(const RE::TESEquipEvent* a_event);

	float CalculateEquippedWeight(RE::Actor* a_actor);
	float GetOrCreateCachedWeight(RE::Actor* a_actor);

	void Revert();

	static void RegisterEvents();

};
