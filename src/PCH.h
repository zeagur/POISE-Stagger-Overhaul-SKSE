#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include "SimpleIni.h"
#include "xbyak/xbyak.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <toml++/toml.h>
#include <shared_mutex>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <ctime>

#include "Logger.h"
#include "Loki_PluginTools.h"
#include "POISE/TrueHUDAPI.h"
#include "POISE/TrueHUDControl.h"
#include "ActorCache.h"


#define TRUEHUD_API_COMMONLIB

using namespace std::literals;

namespace stl
{
	using namespace SKSE::stl;
	template <class T>
	void write_thunk_call(const std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);

		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class F, size_t index, class T>
	void write_vfunc()
	{
		REL::Relocation<> vtbl{ F::VTABLE[index] };
		T::func = vtbl.write_vfunc(T::size, T::thunk);
	}

	template <class F, class T>
	void write_vfunc()
	{
		write_vfunc<F, 0, T>();
	}
}

#ifdef SKSE_SUPPORT_XBYAK
[[nodiscard]] void* allocate(Xbyak::CodeGenerator& a_code);
#endif

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#else
#	define OFFSET(se, ae) se
#endif
