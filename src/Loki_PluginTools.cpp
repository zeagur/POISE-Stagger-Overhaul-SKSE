#include "Loki_PluginTools.h"

/* 
   needed because *i* fucked up my Xbyak stuff
*/
void* Loki::PluginTools::CodeAllocation(Xbyak::CodeGenerator& a_code, SKSE::Trampoline* t_ptr) {

    auto result = t_ptr->allocate(a_code.getSize());
    std::memcpy(result, a_code.getCode(), a_code.getSize());
    return result;

}

/*
    leftover from Paraglider, kept it just in case
*/
float Loki::PluginTools::lerp(float a, float b, float f) {

    return a + f * (b - a);

}

/*
    Experimenting with keywords
*/
bool Loki::PluginTools::WeaponHasKeyword(RE::TESObjectWEAP* a_weap, RE::BSFixedString a_editorID) {

    if (a_weap->keywords) {
        for (std::uint32_t idx = 0; idx < a_weap->numKeywords; ++idx) {
            if (a_weap->keywords[idx] && a_weap->keywords[idx]->formEditorID == a_editorID) {
                return true;
            }
        }
    }

    return false;

}
