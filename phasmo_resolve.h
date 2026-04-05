#pragma once

#include "phasmo_helpers.h"
#include <vector>

namespace PhasmoResolve
{
    constexpr int32_t CAMERA_EYE_LEFT = 0;
    constexpr int32_t CAMERA_EYE_RIGHT = 1;
    constexpr int32_t CAMERA_EYE_MONO = 2;
    constexpr int32_t FIND_OBJECTS_INACTIVE_EXCLUDE = 0;
    constexpr int32_t FIND_OBJECTS_INACTIVE_INCLUDE = 1;
    constexpr int32_t FIND_OBJECTS_SORT_NONE = 0;

    struct ClassRef;

    using ClassGetFieldsFn = void* (*)(void*, void**);
    using FieldGetNameFn = const char* (*)(void*);
    using FieldGetOffsetFn = int32_t (*)(void*);
    using FieldStaticGetValueFn = void (*)(void*, void*);
    using FieldStaticSetValueFn = void (*)(void*, void*);
    using ComponentGetTransformFn = void* (*)(void*, const void*);
    using ComponentGetGameObjectFn = void* (*)(void*, const void*);
    using TransformGetPositionFn = Phasmo::Vec3 (*)(void*, const void*);
    using ComponentGetComponentTypeFn = void* (*)(void*, void*, const void*);
    using ComponentGetComponentInChildrenTypeFn = void* (*)(void*, void*, bool, const void*);
    using RendererGetBoundsFn = Phasmo::Bounds (*)(void*, const void*);
    using AnimatorGetBoneTransformFn = void* (*)(void*, int32_t, const void*);
    using SkinnedMeshGetRootBoneFn = void* (*)(void*, const void*);
    using SkinnedMeshGetBonesFn = void* (*)(void*, const void*);
    using CameraGetMainFn = void* (*)(const void*);
    using CameraGetIntFn = int32_t (*)(void*, const void*);
    using CameraWorldToScreenPointFn = Phasmo::Vec3 (*)(void*, Phasmo::Vec3, const void*);
    using CameraWorldToScreenPointInjectedFn = void (*)(void*, const Phasmo::Vec3*, int32_t, Phasmo::Vec3*, const void*);
    using ObjectFindObjectsOfTypeFn = Phasmo::Il2CppArray* (*)(void*, int32_t, int32_t, const void*);

    inline Phasmo::Il2CppArray* TryFindObjectsByTypeRaw(
        ObjectFindObjectsOfTypeFn fn,
        Il2CppObject* typeObject,
        int32_t findObjectsInactive = FIND_OBJECTS_INACTIVE_INCLUDE,
        int32_t sortMode = FIND_OBJECTS_SORT_NONE)
    {
        if (!fn || !typeObject)
            return nullptr;

        __try {
            return fn(typeObject, findObjectsInactive, sortMode, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    struct FieldRef
    {
        void* address = nullptr;
        int32_t offset = 0;
        bool isStatic = false;

        bool Valid() const
        {
            return address != nullptr;
        }

        template<typename T>
        T Get(void* instance) const
        {
            if (!Valid())
                return T{};

            if (isStatic) {
                T value{};
                FieldStaticGetValueFn fn = nullptr;
                fn = reinterpret_cast<FieldStaticGetValueFn>(Phasmo_GetIl2CppExport("il2cpp_field_static_get_value"));
                if (!fn)
                    return T{};
                __try {
                    fn(address, &value);
                    return value;
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    return T{};
                }
            }

            if (!instance)
                return T{};

            uint8_t* ptr = reinterpret_cast<uint8_t*>(instance) + offset;
            return Phasmo_Safe(ptr, sizeof(T)) ? *reinterpret_cast<T*>(ptr) : T{};
        }

        template<typename T>
        bool Set(void* instance, const T& value) const
        {
            if (!Valid())
                return false;

            if (isStatic) {
                FieldStaticSetValueFn fn = nullptr;
                fn = reinterpret_cast<FieldStaticSetValueFn>(Phasmo_GetIl2CppExport("il2cpp_field_static_set_value"));
                if (!fn)
                    return false;
                __try {
                    fn(address, const_cast<T*>(&value));
                    return true;
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    return false;
                }
            }

            if (!instance)
                return false;

            uint8_t* ptr = reinterpret_cast<uint8_t*>(instance) + offset;
            if (!Phasmo_Safe(ptr, sizeof(T)))
                return false;

            *reinterpret_cast<T*>(ptr) = value;
            return true;
        }
    };

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

    inline ClassRef GetClass(const char* assemblyName, const char* namespaze, const char* name);

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

        FieldRef GetField(const char* name) const
        {
            if (!Valid() || !name)
                return {};

            ClassGetFieldsFn getFields = reinterpret_cast<ClassGetFieldsFn>(Phasmo_GetIl2CppExport("il2cpp_class_get_fields"));
            FieldGetNameFn getName = reinterpret_cast<FieldGetNameFn>(Phasmo_GetIl2CppExport("il2cpp_field_get_name"));
            FieldGetOffsetFn getOffset = reinterpret_cast<FieldGetOffsetFn>(Phasmo_GetIl2CppExport("il2cpp_field_get_offset"));
            if (!getFields || !getName || !getOffset)
                return {};

            void* iter = nullptr;
            void* field = nullptr;
            while ((field = getFields(klass, &iter)) != nullptr) {
                const char* fieldName = getName(field);
                if (!fieldName || strcmp(fieldName, name) != 0)
                    continue;

                const int32_t fieldOffset = getOffset(field);
                return { field, fieldOffset, fieldOffset <= 0 };
            }

            return {};
        }

        template<typename T>
        T GetValue(void* instance, const char* fieldName) const
        {
            return GetField(fieldName).template Get<T>(instance);
        }

        template<typename T>
        bool SetValue(void* instance, const char* fieldName, const T& value) const
        {
            return GetField(fieldName).template Set<T>(instance, value);
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

        template<typename T>
        std::vector<T*> FindObjectsByType() const
        {
            std::vector<T*> result;
            if (!Valid())
                return result;

            static ObjectFindObjectsOfTypeFn findObjectsFn = nullptr;
            if (!findObjectsFn) {
                MethodRef method = PhasmoResolve::GetClass("UnityEngine.CoreModule", "UnityEngine", "Object").GetMethod("FindObjectsOfType", 1);
                findObjectsFn = method.Cast<ObjectFindObjectsOfTypeFn>();
            }

            if (!findObjectsFn)
                return FindObjectsOfTypeAll<T>();

            Il2CppObject* typeObject = GetTypeObject();
            if (!typeObject)
                return result;

            Phasmo::Il2CppArray* arr = TryFindObjectsByTypeRaw(
                findObjectsFn,
                typeObject,
                FIND_OBJECTS_INACTIVE_INCLUDE,
                FIND_OBJECTS_SORT_NONE);

            if (!arr || !Phasmo_Safe(arr, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
                return FindObjectsOfTypeAll<T>();

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

    inline void* GetUnityTypeObject(const char* assemblyName, const char* namespaze, const char* name)
    {
        ClassRef klass = GetClass(assemblyName, namespaze, name);
        return klass.Valid() ? klass.GetTypeObject() : nullptr;
    }

    inline ComponentGetTransformFn ResolveComponentGetTransform()
    {
        static ComponentGetTransformFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<ComponentGetTransformFn>("UnityEngine", "Component", "get_transform", 0, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_Component_get_transform)
                fn = reinterpret_cast<ComponentGetTransformFn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Component_get_transform);
        }
        return fn;
    }

    inline ComponentGetGameObjectFn ResolveComponentGetGameObject()
    {
        static ComponentGetGameObjectFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<ComponentGetGameObjectFn>("UnityEngine", "Component", "get_gameObject", 0, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_Component_get_gameObject)
                fn = reinterpret_cast<ComponentGetGameObjectFn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Component_get_gameObject);
        }
        return fn;
    }

    inline TransformGetPositionFn ResolveTransformGetPosition()
    {
        static TransformGetPositionFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<TransformGetPositionFn>("UnityEngine", "Transform", "get_position", 0, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_Transform_get_position)
                fn = reinterpret_cast<TransformGetPositionFn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Transform_get_position);
        }
        return fn;
    }

    inline ComponentGetComponentTypeFn ResolveGetComponent()
    {
        static ComponentGetComponentTypeFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<ComponentGetComponentTypeFn>("UnityEngine", "Component", "GetComponent", 1, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_Component_GetComponent_Type)
                fn = reinterpret_cast<ComponentGetComponentTypeFn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Component_GetComponent_Type);
        }
        return fn;
    }

    inline ComponentGetComponentInChildrenTypeFn ResolveGetComponentInChildren()
    {
        static ComponentGetComponentInChildrenTypeFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<ComponentGetComponentInChildrenTypeFn>("UnityEngine", "Component", "GetComponentInChildren", 2, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_Component_GetComponentInChildren_Type)
                fn = reinterpret_cast<ComponentGetComponentInChildrenTypeFn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Component_GetComponentInChildren_Type);
        }
        return fn;
    }

    inline RendererGetBoundsFn ResolveRendererGetBounds()
    {
        static RendererGetBoundsFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<RendererGetBoundsFn>("UnityEngine", "Renderer", "get_bounds", 0, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_Renderer_get_bounds)
                fn = reinterpret_cast<RendererGetBoundsFn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Renderer_get_bounds);
        }
        return fn;
    }

    inline AnimatorGetBoneTransformFn ResolveAnimatorGetBoneTransform()
    {
        static AnimatorGetBoneTransformFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<AnimatorGetBoneTransformFn>("UnityEngine", "Animator", "GetBoneTransform", 1, "UnityEngine.AnimationModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_Animator_GetBoneTransform)
                fn = reinterpret_cast<AnimatorGetBoneTransformFn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Animator_GetBoneTransform);
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_Animator_GetBoneTransformInternal)
                fn = reinterpret_cast<AnimatorGetBoneTransformFn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_Animator_GetBoneTransformInternal);
        }
        return fn;
    }

    inline SkinnedMeshGetRootBoneFn ResolveSkinnedMeshGetRootBone()
    {
        static SkinnedMeshGetRootBoneFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<SkinnedMeshGetRootBoneFn>("UnityEngine", "SkinnedMeshRenderer", "get_rootBone", 0, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_SkinnedMeshRenderer_get_rootBone)
                fn = reinterpret_cast<SkinnedMeshGetRootBoneFn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_SkinnedMeshRenderer_get_rootBone);
        }
        return fn;
    }

    inline SkinnedMeshGetBonesFn ResolveSkinnedMeshGetBones()
    {
        static SkinnedMeshGetBonesFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<SkinnedMeshGetBonesFn>("UnityEngine", "SkinnedMeshRenderer", "get_bones", 0, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_UnityEngine_SkinnedMeshRenderer_get_bones)
                fn = reinterpret_cast<SkinnedMeshGetBonesFn>(Phasmo_Base() + Phasmo::RVA_UnityEngine_SkinnedMeshRenderer_get_bones);
        }
        return fn;
    }

    inline CameraGetMainFn ResolveCameraGetMain()
    {
        static CameraGetMainFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<CameraGetMainFn>("UnityEngine", "Camera", "get_main", 0, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_Camera_get_main)
                fn = reinterpret_cast<CameraGetMainFn>(Phasmo_Base() + Phasmo::RVA_Camera_get_main);
        }
        return fn;
    }

    inline CameraGetIntFn ResolveCameraGetPixelWidth()
    {
        static CameraGetIntFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<CameraGetIntFn>("UnityEngine", "Camera", "get_pixelWidth", 0, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_Camera_get_pixelWidth)
                fn = reinterpret_cast<CameraGetIntFn>(Phasmo_Base() + Phasmo::RVA_Camera_get_pixelWidth);
        }
        return fn;
    }

    inline CameraGetIntFn ResolveCameraGetPixelHeight()
    {
        static CameraGetIntFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<CameraGetIntFn>("UnityEngine", "Camera", "get_pixelHeight", 0, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_Camera_get_pixelHeight)
                fn = reinterpret_cast<CameraGetIntFn>(Phasmo_Base() + Phasmo::RVA_Camera_get_pixelHeight);
        }
        return fn;
    }

    inline CameraWorldToScreenPointFn ResolveCameraWorldToScreen()
    {
        static CameraWorldToScreenPointFn fn = nullptr;
        if (!fn) {
            fn = Phasmo_ResolveMethod<CameraWorldToScreenPointFn>("UnityEngine", "Camera", "WorldToScreenPoint", 1, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base() && Phasmo::RVA_Camera_WorldToScreenPoint)
                fn = reinterpret_cast<CameraWorldToScreenPointFn>(Phasmo_Base() + Phasmo::RVA_Camera_WorldToScreenPoint);
        }
        return fn;
    }

    inline CameraWorldToScreenPointInjectedFn ResolveCameraWorldToScreenInjected()
    {
        static CameraWorldToScreenPointInjectedFn fn = nullptr;
        if (!fn && Phasmo_Base() && Phasmo::RVA_Camera_WorldToScreenPoint_Injected)
            fn = reinterpret_cast<CameraWorldToScreenPointInjectedFn>(Phasmo_Base() + Phasmo::RVA_Camera_WorldToScreenPoint_Injected);
        return fn;
    }

    inline void* GetTransform(void* component)
    {
        if (!component || !Phasmo_Safe(component, 0x20))
            return nullptr;

        ComponentGetTransformFn fn = ResolveComponentGetTransform();
        if (!fn)
            return nullptr;

        __try {
            return fn(component, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    inline Phasmo::Vec3 GetPosition(void* transform)
    {
        if (!transform || !Phasmo_Safe(transform, 0x20))
            return { 0.0f, 0.0f, 0.0f };

        TransformGetPositionFn fn = ResolveTransformGetPosition();
        if (!fn)
            return { 0.0f, 0.0f, 0.0f };

        __try {
            return fn(transform, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return { 0.0f, 0.0f, 0.0f };
        }
    }

    inline void* GetGameObject(void* component)
    {
        if (!component || !Phasmo_Safe(component, 0x20))
            return nullptr;

        ComponentGetGameObjectFn fn = ResolveComponentGetGameObject();
        if (!fn)
            return nullptr;

        __try {
            return fn(component, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    inline void* GetComponent(void* component, void* typeObject)
    {
        if (!component || !typeObject || !Phasmo_Safe(component, 0x20))
            return nullptr;

        ComponentGetComponentTypeFn fn = ResolveGetComponent();
        if (!fn)
            return nullptr;

        __try {
            return fn(component, typeObject, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    inline void* GetComponentInChildren(void* component, void* typeObject, bool includeInactive = true)
    {
        if (!component || !typeObject || !Phasmo_Safe(component, 0x20))
            return nullptr;

        ComponentGetComponentInChildrenTypeFn fn = ResolveGetComponentInChildren();
        if (!fn)
            return nullptr;

        __try {
            return fn(component, typeObject, includeInactive, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    inline void* GetAnimatorBoneTransform(void* animator, int32_t boneId)
    {
        if (!animator || !Phasmo_Safe(animator, 0x40))
            return nullptr;

        AnimatorGetBoneTransformFn fn = ResolveAnimatorGetBoneTransform();
        if (!fn)
            return nullptr;

        __try {
            return fn(animator, boneId, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    inline bool GetRendererBounds(void* renderer, Phasmo::Bounds& outBounds)
    {
        if (!renderer || !Phasmo_Safe(renderer, 0x40))
            return false;

        RendererGetBoundsFn fn = ResolveRendererGetBounds();
        if (!fn)
            return false;

        __try {
            outBounds = fn(renderer, nullptr);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline void* GetSkinnedMeshRootBone(void* renderer)
    {
        if (!renderer || !Phasmo_Safe(renderer, 0x40))
            return nullptr;

        SkinnedMeshGetRootBoneFn fn = ResolveSkinnedMeshGetRootBone();
        if (!fn)
            return nullptr;

        __try {
            return fn(renderer, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    inline void* GetSkinnedMeshBones(void* renderer)
    {
        if (!renderer || !Phasmo_Safe(renderer, 0x40))
            return nullptr;

        SkinnedMeshGetBonesFn fn = ResolveSkinnedMeshGetBones();
        if (!fn)
            return nullptr;

        __try {
            return fn(renderer, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    inline void* GetMainCamera()
    {
        CameraGetMainFn fn = ResolveCameraGetMain();
        if (!fn)
            return nullptr;

        __try {
            return fn(nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    inline bool WorldToScreen(void* camera, const Phasmo::Vec3& world, Phasmo::Vec3& screen)
    {
        if (!camera || !Phasmo_Safe(camera, 0x40))
            return false;

        CameraWorldToScreenPointFn directFn = ResolveCameraWorldToScreen();
        if (directFn) {
            __try {
                screen = directFn(camera, world, nullptr);
                return screen.z > 0.01f;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        CameraWorldToScreenPointInjectedFn fn = ResolveCameraWorldToScreenInjected();
        if (!fn)
            return false;

        __try {
            fn(camera, &world, CAMERA_EYE_MONO, &screen, nullptr);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline int32_t GetCameraPixelWidth(void* camera)
    {
        if (!camera || !Phasmo_Safe(camera, 0x40))
            return 0;

        CameraGetIntFn fn = ResolveCameraGetPixelWidth();
        if (!fn)
            return 0;

        __try {
            return fn(camera, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    inline int32_t GetCameraPixelHeight(void* camera)
    {
        if (!camera || !Phasmo_Safe(camera, 0x40))
            return 0;

        CameraGetIntFn fn = ResolveCameraGetPixelHeight();
        if (!fn)
            return 0;

        __try {
            return fn(camera, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }
}
