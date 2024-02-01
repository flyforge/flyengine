#pragma once

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/GameEngineDLL.h>
struct plMsgSetPlaying;

using plPropertyAnimComponentManager = plComponentManagerSimple<class plPropertyAnimComponent, plComponentUpdateType::WhenSimulating>;

/// \brief Animates properties on other objects and components according to the property animation resource
///
/// Notes:
///  - There is no messages to change speed, simply modify the speed property.
class PL_GAMEENGINE_DLL plPropertyAnimComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plPropertyAnimComponent, plComponent, plPropertyAnimComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plPropertyAnimComponent

public:
  plPropertyAnimComponent();
  ~plPropertyAnimComponent();

  void SetPropertyAnimFile(const char* szFile); // [ property ]
  const char* GetPropertyAnimFile() const;      // [ property ]

  void SetPropertyAnim(const plPropertyAnimResourceHandle& hResource);                                     // [ property ]
  PL_ALWAYS_INLINE const plPropertyAnimResourceHandle& GetPropertyAnim() const { return m_hPropertyAnim; } // [ property ]

  /// \brief Sets the animation playback range and resets the playing position to the range start position. Also activates the component if it isn't.
  void PlayAnimationRange(plTime rangeLow, plTime rangeHigh); // [ scriptable ]

  /// \brief Pauses or resumes animation playback. Does not reset any state.
  void OnMsgSetPlaying(plMsgSetPlaying& ref_msg); // [ msg handler ]

  plEnum<plPropertyAnimMode> m_AnimationMode; // [ property ]
  plTime m_RandomOffset;                      // [ property ]
  float m_fSpeed = 1.0f;                      // [ property ]
  plTime m_AnimationRangeLow;                 // [ property ]
  plTime m_AnimationRangeHigh;                // [ property ]
  bool m_bPlaying = true;                     // [ property ]

protected:
  plEventMessageSender<plMsgAnimationReachedEnd> m_ReachedEndMsgSender; // [ event ]
  plEventMessageSender<plMsgGenericEvent> m_EventTrackMsgSender;        // [ event ]

  struct Binding
  {
    const plAbstractMemberProperty* m_pMemberProperty = nullptr;
    mutable void* m_pObject = nullptr; // needs to be updated in case components / objects get relocated in memory
  };

  struct FloatBinding : public Binding
  {
    const plFloatPropertyAnimEntry* m_pAnimation[4] = {nullptr, nullptr, nullptr, nullptr};
  };

  struct ComponentFloatBinding : public FloatBinding
  {
    plComponentHandle m_hComponent;
  };

  struct GameObjectBinding : public FloatBinding
  {
    plGameObjectHandle m_hObject;
  };

  struct ColorBinding : public Binding
  {
    plComponentHandle m_hComponent;
    const plColorPropertyAnimEntry* m_pAnimation = nullptr;
  };

  void Update();
  void CreatePropertyBindings();
  void CreateGameObjectBinding(const plFloatPropertyAnimEntry* pAnim, const plRTTI* pRtti, void* pObject, const plGameObjectHandle& hGameObject);
  void CreateFloatPropertyBinding(const plFloatPropertyAnimEntry* pAnim, const plRTTI* pRtti, void* pObject, const plComponentHandle& hComponent);
  void CreateColorPropertyBinding(const plColorPropertyAnimEntry* pAnim, const plRTTI* pRtti, void* pObject, const plComponentHandle& hComponent);
  void ApplyAnimations(const plTime& tDiff);
  void ApplyFloatAnimation(const FloatBinding& binding, plTime lookupTime);
  void ApplySingleFloatAnimation(const FloatBinding& binding, plTime lookupTime);
  void ApplyColorAnimation(const ColorBinding& binding, plTime lookupTime);
  plTime ComputeAnimationLookup(plTime tDiff);
  void EvaluateEventTrack(plTime startTime, plTime endTime);
  void StartPlayback();

  bool m_bReverse = false;

  plTime m_AnimationTime;
  plHybridArray<GameObjectBinding, 4> m_GoFloatBindings;
  plHybridArray<ComponentFloatBinding, 4> m_ComponentFloatBindings;
  plHybridArray<ColorBinding, 4> m_ColorBindings;
  plPropertyAnimResourceHandle m_hPropertyAnim;

  // we do not want to recreate the binding when the resource changes at runtime
  // therefore we use a sharedptr to keep the data around as long as necessary
  // otherwise that would lead to weird state, because the animation would be interrupted at some point
  // and then the new animation would start from there
  // e.g. when the position is animated, objects could jump around the level
  // when the animation resource is reloaded
  // instead we go with one animation state until this component is reset entirely
  // that means you need to restart a level to see the updated animation
  plSharedPtr<plPropertyAnimResourceDescriptor> m_pAnimDesc;
};
