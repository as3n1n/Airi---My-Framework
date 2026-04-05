#pragma once
#include <windows.h>

namespace AntiCheat
{
    void EarlyInit();   // Call from DllMain — PEB unlink, PE erase, debug flags
    bool Setup();       // Call after GA loaded — hooks, enforcer patches
    void Teardown();
    void OnFrame();
}
