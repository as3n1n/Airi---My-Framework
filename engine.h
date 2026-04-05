#pragma once
#include <windows.h>
#include <stdint.h>

extern HMODULE hIl2Cpp;

struct Il2CppDomain;
struct Il2CppThread;
struct Il2CppClass;

inline Il2CppDomain* Il2CppDomainGet()
{
    if (!hIl2Cpp) return nullptr;
    using Fn = Il2CppDomain* (*)();
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(GetProcAddress(hIl2Cpp, "il2cpp_domain_get"));
    return fn ? fn() : nullptr;
}

inline Il2CppThread* Il2CppThreadCurrent()
{
    if (!hIl2Cpp) return nullptr;
    using Fn = Il2CppThread* (*)();
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(GetProcAddress(hIl2Cpp, "il2cpp_thread_current"));
    return fn ? fn() : nullptr;
}

inline Il2CppThread* Il2CppThreadAttach(Il2CppDomain* domain)
{
    if (!hIl2Cpp || !domain) return nullptr;
    using Fn = Il2CppThread* (*)(Il2CppDomain*);
    static Fn fn = nullptr;
    if (!fn)
        fn = reinterpret_cast<Fn>(GetProcAddress(hIl2Cpp, "il2cpp_thread_attach"));
    return fn ? fn(domain) : nullptr;
}

inline bool Il2CppEnsureCurrentThreadAttached()
{
    thread_local bool attached = false;
    if (attached) return true;
    if (!hIl2Cpp) return false;

    if (Il2CppThreadCurrent())
    {
        attached = true;
        return true;
    }

    Il2CppDomain* domain = Il2CppDomainGet();
    if (!domain) return false;

    Il2CppThread* thread = Il2CppThreadAttach(domain);
    if (!thread) return false;

    attached = true;
    return true;
}
