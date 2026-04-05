#pragma once
#include "phasmo_structs.h"
#include "phasmo_offsets.h"
#include "engine.h"
#include <Windows.h>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

struct Il2CppAssembly;
struct Il2CppImage;
struct Il2CppType;
struct MethodInfo;

inline bool Phasmo_Safe(void* ptr, size_t minSize);
inline void* Phasmo_GetIl2CppExport(const char* name);
template<typename Ret, typename... Args>
inline Ret Phasmo_Call(uintptr_t rva, Args... args);

struct Il2CppString
{
    Il2CppObject object;
    int32_t length;
    char16_t chars[1];
};

// ══════════════════════════════════════════════════════════
//  PHASMO_HELPERS.H — Phasmophobia
//  Fonctions d'aide pour accéder aux structures du jeu
// ══════════════════════════════════════════════════════════

// ─── Base GameAssembly ──────────────────────────────────
inline uintptr_t Phasmo_Base()
{
    static uintptr_t base = 0;
    if (!base) {
        base = reinterpret_cast<uintptr_t>(GetModuleHandleA("GameAssembly.dll"));
    }
    return base;
}

// ─── Safe Pointer Check ─────────────────────────────────
inline bool Phasmo_Safe(void* ptr, size_t minSize = 0x10)
{
    if (!ptr) return false;

    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(ptr, &mbi, sizeof(mbi)))
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    if (!(mbi.Protect & (PAGE_READWRITE | PAGE_READONLY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
        return false;

    if (minSize > 0) {
        uintptr_t endAddr = reinterpret_cast<uintptr_t>(ptr) + minSize;
        if (endAddr > reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize)
            return false;
    }

    return true;
}

// ─── Il2Cpp Runtime API ─────────────────────────────────
inline void* Phasmo_GetIl2CppExport(const char* name)
{
    return hIl2Cpp ? reinterpret_cast<void*>(GetProcAddress(hIl2Cpp, name)) : nullptr;
}

inline Il2CppAssembly* Phasmo_DomainAssemblyOpen(Il2CppDomain* domain, const char* assemblyName)
{
    if (!domain || !assemblyName) return nullptr;

    using Fn = Il2CppAssembly * (*)(Il2CppDomain*, const char*);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_domain_assembly_open"));
    return fn ? fn(domain, assemblyName) : nullptr;
}

inline Il2CppImage* Phasmo_AssemblyGetImage(const Il2CppAssembly* assembly)
{
    if (!assembly) return nullptr;

    using Fn = Il2CppImage * (*)(const Il2CppAssembly*);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_assembly_get_image"));
    return fn ? fn(assembly) : nullptr;
}

inline Il2CppClass* Phasmo_ClassFromName(const Il2CppImage* image, const char* namespaze, const char* name)
{
    if (!image || !name) return nullptr;

    using Fn = Il2CppClass * (*)(const Il2CppImage*, const char*, const char*);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_class_from_name"));
    return fn ? fn(image, namespaze ? namespaze : "", name) : nullptr;
}

inline Il2CppObject* Phasmo_ObjectNew(Il2CppClass* klass)
{
    if (!klass) return nullptr;

    using Fn = Il2CppObject * (*)(Il2CppClass*);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_object_new"));
    return fn ? fn(klass) : nullptr;
}

inline void Phasmo_RuntimeObjectInit(Il2CppObject* obj)
{
    if (!obj) return;

    using Fn = void (*)(Il2CppObject*);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_runtime_object_init"));
    if (fn) fn(obj);
}

inline MethodInfo* Phasmo_ClassGetMethodFromName(Il2CppClass* klass, const char* name, int argsCount)
{
    if (!klass || !name) return nullptr;

    using Fn = MethodInfo * (*)(Il2CppClass*, const char*, int);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_class_get_method_from_name"));
    return fn ? fn(klass, name, argsCount) : nullptr;
}

inline Il2CppObject* Phasmo_RuntimeInvoke(const MethodInfo* method, void* obj, void** params, Il2CppObject** exc)
{
    if (!method) return nullptr;

    using Fn = Il2CppObject * (*)(const MethodInfo*, void*, void**, Il2CppObject**);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_runtime_invoke"));
    return fn ? fn(method, obj, params, exc) : nullptr;
}

inline Il2CppClass* Phasmo_GetClass(const char* namespaze, const char* name, const char* assemblyName = "Assembly-CSharp")
{
    if (!hIl2Cpp || !name) return nullptr;

    Il2CppDomain* domain = Il2CppDomainGet();
    if (!domain) return nullptr;

    Il2CppEnsureCurrentThreadAttached();

    Il2CppAssembly* assembly = Phasmo_DomainAssemblyOpen(domain, assemblyName);
    if (!assembly && assemblyName && !strstr(assemblyName, ".dll")) {
        char dllName[MAX_PATH]{};
        const int written = snprintf(dllName, sizeof(dllName), "%s.dll", assemblyName);
        if (written > 0 && written < static_cast<int>(sizeof(dllName)))
            assembly = Phasmo_DomainAssemblyOpen(domain, dllName);
    }
    if (!assembly) return nullptr;

    Il2CppImage* image = Phasmo_AssemblyGetImage(assembly);
    return Phasmo_ClassFromName(image, namespaze, name);
}

inline const MethodInfo* Phasmo_GetMethodInfo(
    const char* namespaze,
    const char* className,
    const char* methodName,
    int argsCount,
    const char* assemblyName = "Assembly-CSharp")
{
    if (!className || !methodName)
        return nullptr;

    Il2CppClass* klass = Phasmo_GetClass(namespaze, className, assemblyName);
    if (!klass)
        return nullptr;

    return Phasmo_ClassGetMethodFromName(klass, methodName, argsCount);
}

inline void* Phasmo_GetMethodPointer(
    const char* namespaze,
    const char* className,
    const char* methodName,
    int argsCount,
    const char* assemblyName = "Assembly-CSharp")
{
    const MethodInfo* method = Phasmo_GetMethodInfo(namespaze, className, methodName, argsCount, assemblyName);
    if (!method || !Phasmo_Safe(const_cast<MethodInfo*>(method), sizeof(MethodInfo)))
        return nullptr;

    return reinterpret_cast<void*>(method->methodPointer);
}

template<typename Fn>
inline Fn Phasmo_ResolveMethod(
    const char* namespaze,
    const char* className,
    const char* methodName,
    int argsCount,
    const char* assemblyName = "Assembly-CSharp")
{
    return reinterpret_cast<Fn>(Phasmo_GetMethodPointer(namespaze, className, methodName, argsCount, assemblyName));
}

inline Il2CppType* Phasmo_ClassGetType(Il2CppClass* klass)
{
    if (!klass) return nullptr;

    using Fn = Il2CppType * (*)(Il2CppClass*);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_class_get_type"));
    return fn ? fn(klass) : nullptr;
}

inline Il2CppObject* Phasmo_TypeGetObject(Il2CppType* type)
{
    if (!type) return nullptr;

    using Fn = Il2CppObject * (*)(Il2CppType*);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_type_get_object"));
    return fn ? fn(type) : nullptr;
}

// Get static fields pointer from class
inline void* Phasmo_GetStaticFields(Il2CppClass* klass)
{
    if (!klass) return nullptr;

    if (!Phasmo_Safe(klass, offsetof(Il2CppClass, static_fields) + sizeof(void*)))
        return nullptr;
    return klass->static_fields;
}

inline Il2CppClass* Phasmo_GetSystemClass(const char* name)
{
    if (!name)
        return nullptr;

    Il2CppClass* klass = Phasmo_GetClass("System", name, "mscorlib");
    if (!klass)
        klass = Phasmo_GetClass("System", name, "System.Private.CoreLib");
    return klass;
}

inline std::string Phasmo_StringToUtf8(Il2CppString* str)
{
    if (!str || str->length <= 0)
        return {};

    const size_t headerSize = offsetof(Il2CppString, chars);
    const size_t totalBytes = headerSize + static_cast<size_t>(str->length) * sizeof(char16_t);
    if (!Phasmo_Safe(str, totalBytes))
        return {};

    const wchar_t* wide = reinterpret_cast<const wchar_t*>(reinterpret_cast<uint8_t*>(str) + headerSize);
    const int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wide, str->length, nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 0)
        return {};

    std::string result(static_cast<size_t>(utf8Len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, str->length, result.data(), utf8Len, nullptr, nullptr);
    return result;
}

inline Il2CppString* Phasmo_StringNew(const char* src)
{
    if (!src) return nullptr;

    using Fn = Il2CppString * (*)(const char*);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_string_new"));
    return fn ? fn(src) : nullptr;
}

inline Phasmo::Il2CppArray* Phasmo_ArrayNew(Il2CppClass* elementClass, int32_t length)
{
    if (!elementClass || length < 0)
        return nullptr;

    using Fn = Phasmo::Il2CppArray* (*)(Il2CppClass*, size_t);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_array_new"));
    return fn ? fn(elementClass, static_cast<size_t>(length)) : nullptr;
}

inline Il2CppObject* Phasmo_ValueBox(Il2CppClass* klass, const void* data)
{
    if (!klass || !data)
        return nullptr;

    using Fn = Il2CppObject * (*)(Il2CppClass*, void*);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(Phasmo_GetIl2CppExport("il2cpp_value_box"));
    return fn ? fn(klass, const_cast<void*>(data)) : nullptr;
}

inline Phasmo::Il2CppArray* Phasmo_NewObjectArray(int32_t length)
{
    Il2CppClass* objectClass = Phasmo_GetSystemClass("Object");
    return objectClass ? Phasmo_ArrayNew(objectClass, length) : nullptr;
}

inline Il2CppObject* Phasmo_BoxInt32(int32_t value)
{
    Il2CppClass* intClass = Phasmo_GetSystemClass("Int32");
    return intClass ? Phasmo_ValueBox(intClass, &value) : nullptr;
}

inline Il2CppObject* Phasmo_BoxBool(bool value)
{
    Il2CppClass* boolClass = Phasmo_GetSystemClass("Boolean");
    return boolClass ? Phasmo_ValueBox(boolClass, &value) : nullptr;
}

inline bool Phasmo_UnboxInt32(Il2CppObject* obj, int32_t* outValue)
{
    if (!obj || !outValue)
        return false;

    uint8_t* data = reinterpret_cast<uint8_t*>(obj) + sizeof(Il2CppObject);
    *outValue = *reinterpret_cast<int32_t*>(data);
    return true;
}

inline bool Phasmo_ArraySetObject(Phasmo::Il2CppArray* arr, int32_t idx, Il2CppObject* value)
{
    if (!arr || idx < 0)
        return false;
    if (!Phasmo_Safe(arr, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
        return false;
    if (idx >= arr->max_length)
        return false;

    void** entries = reinterpret_cast<void**>(
        reinterpret_cast<uint8_t*>(arr) + Phasmo::ARRAY_FIRST_ELEMENT);
    entries[idx] = value;
    return true;
}

inline Phasmo::Il2CppArray* Phasmo_FindObjectsOfTypeAll(const char* namespaze, const char* name, const char* assemblyName = "Assembly-CSharp")
{
    Il2CppClass* klass = Phasmo_GetClass(namespaze, name, assemblyName);
    if (!klass) return nullptr;

    Il2CppType* type = Phasmo_ClassGetType(klass);
    Il2CppObject* typeObj = Phasmo_TypeGetObject(type);
    if (!typeObj) return nullptr;

    return Phasmo_Call<Phasmo::Il2CppArray*>(Phasmo::RVA_UnityEngine_Resources_FindObjectsOfTypeAll, typeObj, nullptr);
}

// Find first live instance of a type (fallback when singleton static field isn't ready)
template<typename T>
inline T* Phasmo_FindFirstObjectOfType(const char* namespaze, const char* name, const char* assemblyName = "Assembly-CSharp")
{
    Phasmo::Il2CppArray* arr = Phasmo_FindObjectsOfTypeAll(namespaze, name, assemblyName);
    if (!arr || !Phasmo_Safe(arr, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
        return nullptr;

    const int32_t count = arr->max_length;
    if (count <= 0 || count > 2048)
        return nullptr;

    void** items = reinterpret_cast<void**>(
        reinterpret_cast<uint8_t*>(arr) + Phasmo::ARRAY_FIRST_ELEMENT);

    for (int32_t i = 0; i < count; ++i) {
        void* obj = items[i];
        if (obj && Phasmo_Safe(obj, sizeof(T)))
            return reinterpret_cast<T*>(obj);
    }

    return nullptr;
}

// ─── Singleton Accessor Template ────────────────────────
template<typename T>
inline T* Phasmo_GetSingleton(const char* className, const char* namespaze = "", const char* assemblyName = "Assembly-CSharp")
{
    Il2CppClass* klass = Phasmo_GetClass(namespaze, className, assemblyName);
    if (!klass) return nullptr;

    void* sf = Phasmo_GetStaticFields(klass);
    if (!Phasmo_Safe(sf, 0x08)) return nullptr;

    // Instance is at offset 0x0 in static fields
    T* instance = *reinterpret_cast<T**>(
        reinterpret_cast<uint8_t*>(sf) + Phasmo::STATIC_INSTANCE);
    if (Phasmo_Safe(instance, sizeof(T)))
        return instance;

    return Phasmo_FindFirstObjectOfType<T>(namespaze, className, assemblyName);
}

// ─── Specific Singleton Getters ─────────────────────────
inline Phasmo::GameController* Phasmo_GetGameController()
{
    return Phasmo_GetSingleton<Phasmo::GameController>("GameController");
}

inline Phasmo::LevelController* Phasmo_GetLevelController()
{
    return Phasmo_GetSingleton<Phasmo::LevelController>("LevelController");
}

inline Phasmo::EvidenceController* Phasmo_GetEvidenceController()
{
    return Phasmo_GetSingleton<Phasmo::EvidenceController>("EvidenceController");
}

inline Phasmo::GhostController* Phasmo_GetGhostController()
{
    return Phasmo_GetSingleton<Phasmo::GhostController>("GhostController");
}

inline Phasmo::CursedItemsController* Phasmo_GetCursedItemsController()
{
    return Phasmo_GetSingleton<Phasmo::CursedItemsController>("CursedItemsController");
}

inline Phasmo::MapController* Phasmo_GetMapController()
{
    return Phasmo_GetSingleton<Phasmo::MapController>("MapController");
}

inline Phasmo::Network* Phasmo_GetNetwork()
{
    return Phasmo_GetSingleton<Phasmo::Network>("Network");
}

inline Phasmo::LevelValues* Phasmo_GetLevelValues()
{
    return Phasmo_GetSingleton<Phasmo::LevelValues>("LevelValues");
}

inline Phasmo::ObjectiveManager* Phasmo_GetObjectiveManager()
{
    return Phasmo_GetSingleton<Phasmo::ObjectiveManager>("ObjectiveManager");
}

inline Phasmo::RandomWeather* Phasmo_GetRandomWeather()
{
    return Phasmo_GetSingleton<Phasmo::RandomWeather>("RandomWeather");
}

// ─── Get GhostAI from LevelController ───────────────────
inline Phasmo::GhostAI* Phasmo_GetGhostAI()
{
    Phasmo::LevelController* lc = Phasmo_GetLevelController();
    if (!Phasmo_Safe(lc, 0x48)) return nullptr;

    Phasmo::GhostAI* ghost = lc->ghostAI;
    return Phasmo_Safe(ghost, sizeof(Phasmo::GhostAI)) ? ghost : nullptr;
}

// ─── Get FuseBox from LevelController ───────────────────
inline Phasmo::FuseBox* Phasmo_GetFuseBox()
{
    Phasmo::LevelController* lc = Phasmo_GetLevelController();
    if (!Phasmo_Safe(lc, 0x88)) return nullptr;

    Phasmo::FuseBox* fb = reinterpret_cast<Phasmo::FuseBox*>(lc->fuseBox);
    return Phasmo_Safe(fb, sizeof(Phasmo::FuseBox)) ? fb : nullptr;
}

// ─── Get Doors Array from LevelController ───────────────
inline void* Phasmo_GetDoors()
{
    Phasmo::LevelController* lc = Phasmo_GetLevelController();
    if (!Phasmo_Safe(lc, 0x50)) return nullptr;
    return lc->doors;
}

// ─── Get PlayerSanity from Player ───────────────────────
inline Phasmo::PlayerSanity* Phasmo_GetPlayerSanity(Phasmo::Player* player)
{
    if (!Phasmo_Safe(player, 0xC8)) return nullptr;

    Phasmo::PlayerSanity* ps = reinterpret_cast<Phasmo::PlayerSanity*>(player->playerSanity);
    return Phasmo_Safe(ps, sizeof(Phasmo::PlayerSanity)) ? ps : nullptr;
}

// ─── Get PlayerStamina from Player ──────────────────────
inline Phasmo::PlayerStamina* Phasmo_GetPlayerStamina(Phasmo::Player* player)
{
    if (!Phasmo_Safe(player, 0x110)) return nullptr;

    Phasmo::PlayerStamina* pst = reinterpret_cast<Phasmo::PlayerStamina*>(player->playerStamina);
    return Phasmo_Safe(pst, sizeof(Phasmo::PlayerStamina)) ? pst : nullptr;
}

// ─── Call Game Method by RVA ────────────────────────────
template<typename Ret, typename... Args>
inline Ret Phasmo_Call(uintptr_t rva, Args... args)
{
    uintptr_t base = Phasmo_Base();
    if (!base) return Ret{};

    using Fn = Ret(*)(Args...);
    Fn fn = reinterpret_cast<Fn>(base + rva);
    return fn(args...);
}

// ─── Vector3 Math Helpers ───────────────────────────────
inline Phasmo::Vec3 V3Add(const Phasmo::Vec3& a, const Phasmo::Vec3& b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline Phasmo::Vec3 V3Sub(const Phasmo::Vec3& a, const Phasmo::Vec3& b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline Phasmo::Vec3 V3Scale(const Phasmo::Vec3& v, float s)
{
    return { v.x * s, v.y * s, v.z * s };
}

inline float V3Dot(const Phasmo::Vec3& a, const Phasmo::Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float V3Len(const Phasmo::Vec3& v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Phasmo::Vec3 V3Norm(const Phasmo::Vec3& v)
{
    float len = V3Len(v);
    if (len < 0.0001f) return { 0, 0, 0 };
    return V3Scale(v, 1.0f / len);
}

inline float V3Dist(const Phasmo::Vec3& a, const Phasmo::Vec3& b)
{
    return V3Len(V3Sub(a, b));
}

// ─── Transform Accessors (Unity internal) ───────────────
// Note: These access internal Unity transform data.
// For networked positions, prefer using the game's own methods.
inline Phasmo::Vec3 Phasmo_GetPosition(void* transform)
{
    if (!Phasmo_Safe(transform, 0x20)) return { 0, 0, 0 };

    if (Phasmo_Base() && Phasmo::RVA_UnityEngine_Transform_get_position) {
        using Fn = Phasmo::Vec3 (*)(void*, const void*);
        static Fn fn = nullptr;
        if (!fn)
            fn = reinterpret_cast<Fn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Transform_get_position);

        if (fn) {
            __try {
                return fn(transform, nullptr);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
    }

    if (!Phasmo_Safe(transform, 0xC0)) return { 0, 0, 0 };
    return *reinterpret_cast<Phasmo::Vec3*>(reinterpret_cast<uint8_t*>(transform) + Phasmo::TF_POSITION);
}

inline void Phasmo_SetPosition(void* transform, const Phasmo::Vec3& pos)
{
    if (!Phasmo_Safe(transform, 0x20)) return;

    using Fn = void (*)(void*, Phasmo::Vec3, const void*);
    static Fn fn = nullptr;
    if (!fn) {
        fn = Phasmo_ResolveMethod<Fn>("UnityEngine", "Transform", "set_position", 1, "UnityEngine.CoreModule");
    }

    if (fn) {
        __try {
            fn(transform, pos, nullptr);
            return;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    if (!Phasmo_Safe(transform, 0xC0)) return;

    *reinterpret_cast<Phasmo::Vec3*>(
        reinterpret_cast<uint8_t*>(transform) + Phasmo::TF_POSITION) = pos;
}

// ─── PhotonNetwork Static Access ────────────────────────
inline void* Phasmo_GetPhotonNetworkClient()
{
    Il2CppClass* klass = Phasmo_GetClass("Photon.Pun", "PhotonNetwork", "PhotonUnityNetworking");
    if (!klass) return nullptr;

    void* sf = Phasmo_GetStaticFields(klass);
    if (!Phasmo_Safe(sf, 0x10)) return nullptr;

    return *reinterpret_cast<void**>(
        reinterpret_cast<uint8_t*>(sf) + Phasmo::PN_NETWORKING_CLIENT);
}

namespace PhasmoUnity
{
    struct MethodRef
    {
        const MethodInfo* info = nullptr;

        bool Valid() const
        {
            return info && Phasmo_Safe(const_cast<MethodInfo*>(info), sizeof(MethodInfo)) && info->methodPointer;
        }

        template<typename Fn>
        Fn Cast() const
        {
            return Valid() ? reinterpret_cast<Fn>(info->methodPointer) : nullptr;
        }

        template<typename Ret, typename... Args>
        Ret Invoke(Args... args) const
        {
            using Fn = Ret(*)(Args...);
            Fn fn = Cast<Fn>();
            return fn ? fn(args...) : Ret{};
        }
    };

    struct ClassRef
    {
        Il2CppClass* klass = nullptr;

        bool Valid() const
        {
            return klass != nullptr;
        }

        MethodRef GetMethod(const char* name, int argsCount) const
        {
            return { Valid() ? Phasmo_ClassGetMethodFromName(klass, name, argsCount) : nullptr };
        }

        Il2CppObject* New() const
        {
            return Valid() ? Phasmo_ObjectNew(klass) : nullptr;
        }

        Il2CppType* GetType() const
        {
            return Valid() ? Phasmo_ClassGetType(klass) : nullptr;
        }

        Il2CppObject* GetTypeObject() const
        {
            Il2CppType* type = GetType();
            return type ? Phasmo_TypeGetObject(type) : nullptr;
        }

        template<typename T>
        std::vector<T*> FindObjectsOfTypeAll() const
        {
            std::vector<T*> result;
            if (!Valid())
                return result;

            Il2CppObject* typeObject = GetTypeObject();
            if (!typeObject)
                return result;

            Phasmo::Il2CppArray* arr = Phasmo_Call<Phasmo::Il2CppArray*>(
                Phasmo::RVA_UnityEngine_Resources_FindObjectsOfTypeAll, typeObject, nullptr);
            if (!arr || !Phasmo_Safe(arr, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
                return result;

            const int32_t count = arr->max_length;
            if (count <= 0 || count > 4096)
                return result;

            void** entries = reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(arr) + Phasmo::ARRAY_FIRST_ELEMENT);
            result.reserve(static_cast<size_t>(count));
            for (int32_t i = 0; i < count; ++i) {
                void* obj = entries[i];
                if (obj && Phasmo_Safe(obj, sizeof(T)))
                    result.push_back(reinterpret_cast<T*>(obj));
            }

            return result;
        }
    };

    struct AssemblyRef
    {
        const char* assemblyName = "Assembly-CSharp";

        ClassRef GetClass(const char* namespaze, const char* name) const
        {
            return { Phasmo_GetClass(namespaze, name, assemblyName) };
        }
    };

    inline AssemblyRef GetAssembly(const char* assemblyName)
    {
        return { assemblyName };
    }

    inline ClassRef GetClass(const char* assemblyName, const char* namespaze, const char* name)
    {
        return { Phasmo_GetClass(namespaze, name, assemblyName) };
    }

    inline void* GetComponent(void* component, Il2CppObject* typeObject)
    {
        if (!component || !typeObject || !Phasmo_Safe(component, 0x20))
            return nullptr;

        using Fn = void* (*)(void*, void*, const void*);
        static Fn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<Fn>("UnityEngine", "Component", "GetComponent", 1, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_Component_GetComponent_Type)
                fn = reinterpret_cast<Fn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Component_GetComponent_Type);
        }

        if (!fn)
            return nullptr;

        __try {
            return fn(component, typeObject, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    inline void* GetComponentInChildren(void* component, Il2CppObject* typeObject, bool includeInactive = true)
    {
        if (!component || !typeObject || !Phasmo_Safe(component, 0x20))
            return nullptr;

        using Fn = void* (*)(void*, void*, bool, const void*);
        static Fn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<Fn>("UnityEngine", "Component", "GetComponentInChildren", 2, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_Component_GetComponentInChildren_Type)
                fn = reinterpret_cast<Fn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Component_GetComponentInChildren_Type);
        }

        if (!fn)
            return nullptr;

        __try {
            return fn(component, typeObject, includeInactive, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }
}
