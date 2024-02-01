#pragma once

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Types/UniquePtr.h>
#include <JoltPlugin/JoltPluginDLL.h>

class plJoltMaterial;
struct plSurfaceResourceEvent;
class plJoltDebugRenderer;
class plWorld;

namespace JPH
{
  class JobSystem;
}

class PL_JOLTPLUGIN_DLL plJoltCore
{
public:
  static JPH::JobSystem* GetJoltJobSystem() { return s_pJobSystem.get(); }
  static const plJoltMaterial* GetDefaultMaterial() { return s_pDefaultMaterial; }

  static void DebugDraw(plWorld* pWorld);

#ifdef JPH_DEBUG_RENDERER
  static std::unique_ptr<plJoltDebugRenderer> s_pDebugRenderer;
#endif

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(Jolt, JoltPlugin);

  static void Startup();
  static void Shutdown();

  static void SurfaceResourceEventHandler(const plSurfaceResourceEvent& e);

  static void* JoltMalloc(size_t inSize);
  static void JoltFree(void* inBlock);
  static void* JoltAlignedMalloc(size_t inSize, size_t inAlignment);
  static void JoltAlignedFree(void* inBlock);

  static plJoltMaterial* s_pDefaultMaterial;
  static std::unique_ptr<JPH::JobSystem> s_pJobSystem;

  static plUniquePtr<plProxyAllocator> s_pAllocator;
};
