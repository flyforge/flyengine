#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>
#include <AudioSystemPlugin/Core/AudioSystemMessages.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

/// \brief Base class for audio system component manager which need to update their states (eg. AudioListenerComponent).
template <typename T>
class plAudioSystemComponentManager final : public plComponentManager<T, plBlockStorageType::FreeList>
{
  // plComponentManager

public:
  void Initialize() override;

  // plAudioSystemComponentManager

public:
  explicit plAudioSystemComponentManager(plWorld* pWorld);

private:
  /// \brief A simple update function that iterates over all components and calls Update() on every component
  void Update(const plWorldModule::UpdateContext& context);

  static void UpdateFunctionName(plStringBuilder& out_sName);
};

/// \brief Base class for audio system components.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plAudioSystemComponent, plComponent);

  // plAudioSystemComponent

protected:
  // Dummy method to hide this component in the editor UI.
  virtual void plAudioSystemComponentIsAbstract() = 0;
};

/// \brief Base class for audio system components that depends on an audio proxy component.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemProxyDependentComponent : public plAudioSystemComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plAudioSystemProxyDependentComponent, plAudioSystemComponent);

  // plComponent

public:
  void Initialize() override;
  void OnSimulationStarted() override;
  void Deinitialize() override;

  // plAudiSystemProxyDependentComponent

protected:
  /// \brief Get the ID of the entity referenced by the proxy.
  PLASMA_NODISCARD plAudioSystemDataID GetEntityId() const;

  class plAudioProxyComponent* m_pProxyComponent{nullptr};
};

/// \brief Base class for audio system environment components.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemEnvironmentComponent : public plAudioSystemProxyDependentComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plAudioSystemEnvironmentComponent, plAudioSystemProxyDependentComponent);

  // plAudioSystemEnvironmentComponent

public:
  plAudioSystemEnvironmentComponent();

  void OnActivated() override;
  void OnDeactivated() override;

  /// \brief Gets the environment amount for the specified audio proxy component.
  /// \param pProxyComponent The proxy component for which compute the environment amount.
  /// \return The environment amount.
  PLASMA_NODISCARD virtual float GetEnvironmentAmount(plAudioProxyComponent* pProxyComponent) const = 0;

  /// \brief Gets the ID of the environment in the Audio System.
  PLASMA_NODISCARD plAudioSystemDataID GetEnvironmentId() const;

  /// \brief Gets the distance from the sphere's origin
  /// at which the environment amount will slightly start to decrease.
  PLASMA_NODISCARD virtual float GetMaxDistance() const;

  /// \brief Sets the distance from the sphere's origin at which
  /// the environment amount will slightly start to decrease.
  virtual void SetMaxDistance(float fFadeDistance);

  /// \brief Overrides the computed environment value with the given one.
  /// \param fValue The override value. Use a negative value to cancel any previous override.
  void OverrideEnvironmentAmount(float fValue);

  void OnSetAmount(plMsgAudioSystemSetEnvironmentAmount& msg);

protected:
  float m_fMaxDistance;
  plString m_sEnvironmentName;
  plColor m_ShapeColor;

  bool m_bOverrideValue;
  float m_fOverrideValue;
};

template <typename T>
plAudioSystemComponentManager<T>::plAudioSystemComponentManager(plWorld* pWorld)
  : plComponentManager<T, plBlockStorageType::FreeList>(pWorld)
{
}

template <typename T>
void plAudioSystemComponentManager<T>::Initialize()
{
  plStringBuilder functionName;
  UpdateFunctionName(functionName);

  auto desc = plWorldModule::UpdateFunctionDesc(plWorldModule::UpdateFunction(&plAudioSystemComponentManager<T>::Update, this), functionName);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostTransform; // Should we apply entity transform after game object transform?

  this->RegisterUpdateFunction(desc);
}

template <typename T>
void plAudioSystemComponentManager<T>::Update(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (T* pComponent = it; pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

// static
template <typename T>
void plAudioSystemComponentManager<T>::UpdateFunctionName(plStringBuilder& out_sName)
{
  plStringView sName(PLASMA_SOURCE_FUNCTION);
  const char* szEnd = sName.FindSubString(",");

  if (szEnd != nullptr && sName.StartsWith("plAudioSystemComponentManager<class "))
  {
    const plStringView sChoppedName(sName.GetStartPointer() + plStringUtils::GetStringElementCount("plAudioSystemComponentManager<class "), szEnd);

    PLASMA_ASSERT_DEV(!sChoppedName.IsEmpty(), "Chopped name is empty: '{0}'", sName);

    out_sName = sChoppedName;
    out_sName.Append("::Update");
  }
  else
  {
    out_sName = sName;
  }
}
