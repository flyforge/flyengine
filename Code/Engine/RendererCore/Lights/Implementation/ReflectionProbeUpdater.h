#pragma once

#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

PL_DECLARE_FLAGS(plUInt8, plReflectionProbeUpdaterFlags, SkyLight, HasCustomCubeMap);

/// \brief Renders reflection probes and stores filtered mipmap chains into an atlas texture as well as computing sky irradiance
/// Rendering sky irradiance is optional and only done if m_iIrradianceOutputIndex != -1.
class plReflectionProbeUpdater
{
public:
  /// \brief Defines the target specular reflection probe atlas and index as well as the sky irradiance atlas and index in case the rendered cube map is a sky light.
  struct TargetSlot
  {
    plGALTextureHandle m_hSpecularOutputTexture;   ///< Must be a valid cube map texture array handle.
    plGALTextureHandle m_hIrradianceOutputTexture; ///< Optional. Must be set if m_iIrradianceOutputIndex != -1.
    plInt32 m_iSpecularOutputIndex = -1;           ///< Must be a valid index into the atlas texture.
    plInt32 m_iIrradianceOutputIndex = -1;         ///< If -1, no irradiance is computed.
  };

public:
  plReflectionProbeUpdater();
  ~plReflectionProbeUpdater();

  /// \brief Returns how many new probes can be started this frame.
  /// \param out_updatesFinished Contains the probes that finished last frame.
  /// \return The number of new probes can be started this frame.
  plUInt32 GetFreeUpdateSlots(plDynamicArray<plReflectionProbeRef>& out_updatesFinished);

  /// \brief Starts rendering a new reflection probe.
  /// \param probe The world and probe index to be rendered. Used as an identifier.
  /// \param desc Probe render settings.
  /// \param globalTransform World position to be rendered.
  /// \param target Where the probe should be rendered into.
  /// \return Returns PL_FAILURE if no more free slots are available.
  plResult StartDynamicUpdate(const plReflectionProbeRef& probe, const plReflectionProbeDesc& desc, const plTransform& globalTransform, const TargetSlot& target);

  /// \brief Starts filtering an existing cube map into a new reflection probe.
  /// \param probe The world and probe index to be rendered. Used as an identifier.
  /// \param desc Probe render settings.
  /// \param sourceTexture Cube map that should be filtered into a reflection probe.
  /// \param target Where the probe should be rendered into.
  /// \return Returns PL_FAILURE if no more free slots are available.
  plResult StartFilterUpdate(const plReflectionProbeRef& probe, const plReflectionProbeDesc& desc, plTextureCubeResourceHandle hSourceTexture, const TargetSlot& target);

  /// \brief Cancel a previously started update.
  void CancelUpdate(const plReflectionProbeRef& probe);

  /// \brief Generates update steps. Should be called in PreExtraction phase.
  void GenerateUpdateSteps();

  /// \brief Schedules probe rendering views. Should be called at some point during the extraction phase. Can be called multiple times. It will only do work on the first call after GenerateUpdateSteps.
  void ScheduleUpdateSteps();

private:
  struct ReflectionView
  {
    plViewHandle m_hView;
    plCamera m_Camera;
  };

  struct UpdateStep
  {
    using StorageType = plUInt8;

    enum Enum
    {
      RenderFace0,
      RenderFace1,
      RenderFace2,
      RenderFace3,
      RenderFace4,
      RenderFace5,
      Filter,

      ENUM_COUNT,

      Default = Filter
    };

    static bool IsRenderStep(Enum value) { return value >= UpdateStep::RenderFace0 && value <= UpdateStep::RenderFace5; }
    static Enum NextStep(Enum value) { return static_cast<UpdateStep::Enum>((value + 1) % UpdateStep::ENUM_COUNT); }
  };

  struct ProbeUpdateInfo
  {
    ProbeUpdateInfo();
    ~ProbeUpdateInfo();

    plBitflags<plReflectionProbeUpdaterFlags> m_flags;
    plReflectionProbeRef m_probe;
    plReflectionProbeDesc m_desc;
    plTransform m_globalTransform;
    plTextureCubeResourceHandle m_sourceTexture;
    TargetSlot m_TargetSlot;

    struct Step
    {
      PL_DECLARE_POD_TYPE();

      plUInt8 m_uiViewIndex;
      plEnum<UpdateStep> m_UpdateStep;
    };

    bool m_bInUse = false;
    plEnum<UpdateStep> m_LastUpdateStep;

    plHybridArray<Step, 8> m_UpdateSteps;

    plGALTextureHandle m_hCubemap;
    plGALTextureHandle m_hCubemapProxies[6];
  };

private:
  static void CreateViews(
    plDynamicArray<ReflectionView>& views, plUInt32 uiMaxRenderViews, const char* szNameSuffix, const char* szRenderPipelineResource);
  void CreateReflectionViewsAndResources();

  void ResetProbeUpdateInfo(plUInt32 uiInfo);
  void AddViewToRender(const ProbeUpdateInfo::Step& step, ProbeUpdateInfo& updateInfo);

  bool m_bUpdateStepsFlushed = true;

  plDynamicArray<ReflectionView> m_RenderViews;
  plDynamicArray<ReflectionView> m_FilterViews;

  // Active Dynamic Updates
  plDynamicArray<plUniquePtr<ProbeUpdateInfo>> m_DynamicUpdates;
  plHybridArray<plReflectionProbeRef, 4> m_FinishedLastFrame;
};
