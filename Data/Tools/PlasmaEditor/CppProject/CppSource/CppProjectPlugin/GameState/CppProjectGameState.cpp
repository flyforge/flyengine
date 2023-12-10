#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <CppProjectPlugin/GameState/CppProjectGameState.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>

plCVarBool cvar_DebugDisplay("CppProject.DebugDisplay", false, plCVarFlags::Default, "Whether the game should display debug geometry.");

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(CppProjectGameState, 1, plRTTIDefaultAllocator<CppProjectGameState>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

CppProjectGameState::CppProjectGameState() = default;
CppProjectGameState::~CppProjectGameState() = default;

void CppProjectGameState::OnActivation(plWorld* pWorld, const plTransform* pStartPosition)
{
  PLASMA_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, pStartPosition);
}

void CppProjectGameState::OnDeactivation()
{
  PLASMA_LOG_BLOCK("GameState::Deactivate");

  SUPER::OnDeactivation();
}

void CppProjectGameState::AfterWorldUpdate()
{
  SUPER::AfterWorldUpdate();

  if (cvar_DebugDisplay)
  {
    plDebugRenderer::DrawLineSphere(m_pMainWorld, plBoundingSphere(plVec3::ZeroVector(), 1.0f), plColor::Orange);
  }

  plDebugRenderer::Draw2DText(m_pMainWorld, "Press 'O' to spawn objects", plVec2I32(10, 10), plColor::White);
  plDebugRenderer::Draw2DText(m_pMainWorld, "Press 'P' to remove objects", plVec2I32(10, 30), plColor::White);
}

void CppProjectGameState::BeforeWorldUpdate()
{
  PLASMA_LOCK(m_pMainWorld->GetWriteMarker());
}

plGameStatePriority CppProjectGameState::DeterminePriority(plWorld* pWorld) const
{
  return plGameStatePriority::Default;
}

void CppProjectGameState::ConfigureMainWindowInputDevices(plWindow* pWindow)
{
  SUPER::ConfigureMainWindowInputDevices(pWindow);

  // setup devices here
}

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  plInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = true;
  cfg.m_sInputSlotTrigger[0] = szKey1;
  cfg.m_sInputSlotTrigger[1] = szKey2;
  cfg.m_sInputSlotTrigger[2] = szKey3;

  plInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void CppProjectGameState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();

  RegisterInputAction("CppProjectPlugin", "SpawnObject", plInputSlot_KeyO, plInputSlot_Controller0_ButtonA, plInputSlot_MouseButton2);
  RegisterInputAction("CppProjectPlugin", "DeleteObject", plInputSlot_KeyP, plInputSlot_Controller0_ButtonB);
}

void CppProjectGameState::ProcessInput()
{
  SUPER::ProcessInput();

  plWorld* pWorld = m_pMainWorld;

  if (plInputManager::GetInputActionState("CppProjectPlugin", "SpawnObject") == plKeyState::Pressed)
  {
    const plVec3 pos = GetMainCamera()->GetCenterPosition() + GetMainCamera()->GetCenterDirForwards();

    // make sure we are allowed to modify the world
    PLASMA_LOCK(pWorld->GetWriteMarker());

    // create a game object at the desired position
    plGameObjectDesc desc;
    desc.m_LocalPosition = pos;

    plGameObject* pObject = nullptr;
    plGameObjectHandle hObject = pWorld->CreateObject(desc, pObject);

    m_SpawnedObjects.PushBack(hObject);

    // attach a mesh component to the object
    plMeshComponent* pMesh;
    pWorld->GetOrCreateComponentManager<plMeshComponentManager>()->CreateComponent(pObject, pMesh);

    // Set the mesh to use.
    // Here we use a path relative to the project directory.
    // We have to reference the 'transformed' file, not the source file.
    // This would break if the source asset is moved or renamed.
    pMesh->SetMeshFile("AssetCache/Common/Meshes/Sphere.plMesh");

    // here we use the asset GUID to reference the transformed asset
    // we can copy the GUID from the asset browser
    // the GUID is stable even if the source asset gets moved or renamed
    // using asset collections we could also give a nice name like 'Blue Material' to this asset
    plMaterialResourceHandle hMaterial = plResourceManager::LoadResource<plMaterialResource>("{ aa1c5601-bc43-fbf8-4e07-6a3df3af51e7 }");

    // override the mesh material in the first slot with something different
    pMesh->SetMaterial(0, hMaterial);
  }

  if (plInputManager::GetInputActionState("CppProjectPlugin", "DeleteObject") == plKeyState::Pressed)
  {
    if (!m_SpawnedObjects.IsEmpty())
    {
      // make sure we are allowed to modify the world
      PLASMA_LOCK(pWorld->GetWriteMarker());

      plGameObjectHandle hObject = m_SpawnedObjects.PeekBack();
      m_SpawnedObjects.PopBack();

      // this is only for demonstration purposes, removing the object will delete all attached components as well
      plGameObject* pObject = nullptr;
      if (pWorld->TryGetObject(hObject, pObject))
      {
        plMeshComponent* pMesh = nullptr;
        if (pObject->TryGetComponentOfBaseType(pMesh))
        {
          pMesh->DeleteComponent();
        }
      }

      // delete the object, all its children and attached components
      pWorld->DeleteObjectDelayed(hObject);
    }
  }
}

void CppProjectGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // do custom camera setup here
}
