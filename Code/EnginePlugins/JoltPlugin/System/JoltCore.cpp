#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Factory.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Configuration/CVar.h>
#include <Jolt/Core/IssueReporting.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/RegisterTypes.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/Implementation/JoltCustomShapeInfo.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <stdarg.h>

#ifdef JPH_DEBUG_RENDERER
std::unique_ptr<plJoltDebugRenderer> plJoltCore::s_pDebugRenderer;
#endif

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plJoltSteppingMode, 1)
  PL_ENUM_CONSTANTS(plJoltSteppingMode::Variable, plJoltSteppingMode::Fixed, plJoltSteppingMode::SemiFixed)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plOnJoltContact, 1)
  //PL_BITFLAGS_CONSTANT(plOnJoltContact::SendReportMsg),
  PL_BITFLAGS_CONSTANT(plOnJoltContact::ImpactReactions),
  PL_BITFLAGS_CONSTANT(plOnJoltContact::SlideReactions),
  PL_BITFLAGS_CONSTANT(plOnJoltContact::RollXReactions),
  PL_BITFLAGS_CONSTANT(plOnJoltContact::RollYReactions),
  PL_BITFLAGS_CONSTANT(plOnJoltContact::RollZReactions),
PL_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

plJoltMaterial* plJoltCore::s_pDefaultMaterial = nullptr;
std::unique_ptr<JPH::JobSystem> plJoltCore::s_pJobSystem;

plUniquePtr<plProxyAllocator> plJoltCore::s_pAllocator;

plJoltMaterial::plJoltMaterial() = default;
plJoltMaterial::~plJoltMaterial() = default;

static void JoltTraceFunc(const char* szText, ...)
{
  plStringBuilder tmp;

  va_list args;
  va_start(args, szText);
  tmp.SetPrintfArgs(szText, args);
  va_end(args);

  plLog::Dev("Jolt: {}", tmp);
}

#ifdef JPH_ENABLE_ASSERTS

static bool JoltAssertFailed(const char* szInExpression, const char* szInMessage, const char* szInFile, uint32_t inLine)
{
  return plFailedCheck(szInFile, inLine, "Jolt", szInExpression, szInMessage);
};

#endif // JPH_ENABLE_ASSERTS

void plJoltCore::DebugDraw(plWorld* pWorld)
{
#ifdef JPH_DEBUG_RENDERER
  if (s_pDebugRenderer == nullptr)
    return;

  plDebugRenderer::DrawSolidTriangles(pWorld, s_pDebugRenderer->m_Triangles, plColor::White);
  plDebugRenderer::DrawLines(pWorld, s_pDebugRenderer->m_Lines, plColor::White);

  s_pDebugRenderer->m_Triangles.Clear();
  s_pDebugRenderer->m_Lines.Clear();
#endif
}

void* plJoltCore::JoltMalloc(size_t inSize)
{
  return plJoltCore::s_pAllocator->Allocate(inSize, 16);
}

void plJoltCore::JoltFree(void* inBlock)
{
  plJoltCore::s_pAllocator->Deallocate(inBlock);
}

void* plJoltCore::JoltAlignedMalloc(size_t inSize, size_t inAlignment)
{
  return plJoltCore::s_pAllocator->Allocate(inSize, inAlignment);
}

void plJoltCore::JoltAlignedFree(void* inBlock)
{
  plJoltCore::s_pAllocator->Deallocate(inBlock);
}

void plJoltCore::Startup()
{
  s_pAllocator = PL_DEFAULT_NEW(plProxyAllocator, "Jolt-Core", plFoundation::GetAlignedAllocator());

  JPH::Trace = JoltTraceFunc;
  JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed);
  JPH::Allocate = plJoltCore::JoltMalloc;
  JPH::Free = plJoltCore::JoltFree;
  JPH::AlignedAllocate = plJoltCore::JoltAlignedMalloc;
  JPH::AlignedFree = plJoltCore::JoltAlignedFree;

  JPH::Factory::sInstance = new JPH::Factory();

  JPH::RegisterTypes();

  plJoltCustomShapeInfo::sRegister();

  // TODO: custom job system
  s_pJobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

  s_pDefaultMaterial = new plJoltMaterial;
  s_pDefaultMaterial->AddRef();
  JPH::PhysicsMaterial::sDefault = s_pDefaultMaterial;

#ifdef JPH_DEBUG_RENDERER
  s_pDebugRenderer = std::make_unique<plJoltDebugRenderer>();
#endif

  plSurfaceResource::s_Events.AddEventHandler(&plJoltCore::SurfaceResourceEventHandler);
}

void plJoltCore::Shutdown()
{
#ifdef JPH_DEBUG_RENDERER
  s_pDebugRenderer = nullptr;
#endif

  JPH::PhysicsMaterial::sDefault = nullptr;

  s_pDefaultMaterial->Release();
  s_pDefaultMaterial = nullptr;

  s_pJobSystem = nullptr;

  delete JPH::Factory::sInstance;
  JPH::Factory::sInstance = nullptr;

  JPH::Trace = nullptr;

  s_pAllocator.Clear();

  plSurfaceResource::s_Events.RemoveEventHandler(&plJoltCore::SurfaceResourceEventHandler);
}

void plJoltCore::SurfaceResourceEventHandler(const plSurfaceResourceEvent& e)
{
  if (e.m_Type == plSurfaceResourceEvent::Type::Created)
  {
    const auto& desc = e.m_pSurface->GetDescriptor();

    auto pNewMaterial = new plJoltMaterial;
    pNewMaterial->AddRef();
    pNewMaterial->m_pSurface = e.m_pSurface;
    pNewMaterial->m_fRestitution = desc.m_fPhysicsRestitution;
    pNewMaterial->m_fFriction = plMath::Lerp(desc.m_fPhysicsFrictionStatic, desc.m_fPhysicsFrictionDynamic, 0.5f);

    e.m_pSurface->m_pPhysicsMaterialJolt = pNewMaterial;
  }
  else if (e.m_Type == plSurfaceResourceEvent::Type::Destroyed)
  {
    if (e.m_pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      plJoltMaterial* pMaterial = static_cast<plJoltMaterial*>(e.m_pSurface->m_pPhysicsMaterialJolt);
      pMaterial->Release();

      e.m_pSurface->m_pPhysicsMaterialJolt = nullptr;
    }
  }
}


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltCore);

