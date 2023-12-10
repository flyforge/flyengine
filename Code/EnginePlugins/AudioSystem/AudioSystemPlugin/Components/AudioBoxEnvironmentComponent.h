#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>

typedef plAudioSystemComponentManager<class plAudioBoxEnvironmentComponent> plAudioBoxEnvironmentComponentManager;

/// \brief Component that applies environmental effects in a box shape.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioBoxEnvironmentComponent : public plAudioSystemEnvironmentComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAudioBoxEnvironmentComponent, plAudioSystemEnvironmentComponent, plAudioBoxEnvironmentComponentManager);

  // plComponent

public:
  void Initialize() override;
  void Deinitialize() override;
  void SerializeComponent(plWorldWriter& stream) const override;
  void DeserializeComponent(plWorldReader& stream) override;

  // plAudioSystemComponent

protected:
  void plAudioSystemComponentIsAbstract() override {}

  // plAudioSystemEnvironmentComponent

public:
  PLASMA_NODISCARD float GetEnvironmentAmount(plAudioProxyComponent* pProxyComponent) const override;

  // plAudioBoxEnvironmentComponent

public:
  /// \brief Gets the radius of the sphere that
  /// specifies the environment.
  PLASMA_NODISCARD const plVec3& GetHalfExtends() const;

  /// \brief Sets the radius of the sphere that
  /// specifies the environment.
  void SetHalfExtends(const plVec3& vHalfExtends);

protected:
  void Update();

private:
  plVec3 m_vHalfExtends;

  plBoundingBox m_Box;
};
