#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/BakedProbes/BakingInterface.h>
#include <RendererCore/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct plMsgUpdateLocalBounds;
struct plMsgExtractRenderData;
struct plRenderWorldRenderEvent;
class plAbstractObjectNode;

class PLASMA_RENDERERCORE_DLL plBakedProbesComponentManager : public plSettingsComponentManager<class plBakedProbesComponent>
{
public:
  plBakedProbesComponentManager(plWorld* pWorld);
  ~plBakedProbesComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  plMeshResourceHandle m_hDebugSphere;
  plMaterialResourceHandle m_hDebugMaterial;

private:
  void RenderDebug(const plWorldModule::UpdateContext& updateContext);
  void OnRenderEvent(const plRenderWorldRenderEvent& e);
  void CreateDebugResources();
};

class PLASMA_RENDERERCORE_DLL plBakedProbesComponent : public plSettingsComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plBakedProbesComponent, plSettingsComponent, plBakedProbesComponentManager);

public:
  plBakedProbesComponent();
  ~plBakedProbesComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  plBakingSettings m_Settings; // [ property ]

  void SetShowDebugOverlay(bool bShow);                            // [ property ]
  bool GetShowDebugOverlay() const { return m_bShowDebugOverlay; } // [ property ]

  void SetShowDebugProbes(bool bShow);                           // [ property ]
  bool GetShowDebugProbes() const { return m_bShowDebugProbes; } // [ property ]

  void SetUseTestPosition(bool bUse);                            // [ property ]
  bool GetUseTestPosition() const { return m_bUseTestPosition; } // [ property ]

  void SetTestPosition(const plVec3& vPos);                         // [ property ]
  const plVec3& GetTestPosition() const { return m_vTestPosition; } // [ property ]

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg);
  void OnExtractRenderData(plMsgExtractRenderData& ref_msg) const;

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

private:
  void RenderDebugOverlay();
  void OnObjectCreated(const plAbstractObjectNode& node);

  plHashedString m_sProbeTreeResourcePrefix;

  bool m_bShowDebugOverlay = false;
  bool m_bShowDebugProbes = false;
  bool m_bUseTestPosition = false;
  plVec3 m_vTestPosition = plVec3::ZeroVector();

  struct RenderDebugViewTask;
  plSharedPtr<RenderDebugViewTask> m_pRenderDebugViewTask;

  plGALTextureHandle m_hDebugViewTexture;
};
