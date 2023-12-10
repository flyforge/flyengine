#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>

/// \brief Component manager for audio system environment components.
typedef plAudioSystemComponentManager<class plAudioSphereEnvironmentComponent> plAudioSphereEnvironmentComponentManager;

/// \brief Component used to apply environment effects in a sphere shape.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSphereEnvironmentComponent : public plAudioSystemEnvironmentComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAudioSphereEnvironmentComponent, plAudioSystemEnvironmentComponent, plAudioSphereEnvironmentComponentManager);

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

  // plAudioSphereEnvironmentComponent

public:
  plAudioSphereEnvironmentComponent();

  /// \brief Gets the radius of the sphere that
  /// specifies the environment.
  /// \returns The sphere's radius.
  PLASMA_NODISCARD float GetRadius() const;

  /// \brief Sets the radius of the sphere that
  /// specifies the environment.
  /// \param fRadius The sphere's radius.
  void SetRadius(float fRadius);

protected:
  void Update();

private:
  plBoundingSphere m_Sphere;
};
