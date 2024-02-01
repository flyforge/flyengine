#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

using plColorAnimationComponentManager = plComponentManagerSimple<class plColorAnimationComponent, plComponentUpdateType::WhenSimulating>;

/// \brief Samples a color gradient and sends an plMsgSetColor to the object it is attached to
///
/// The color gradient is sampled linearly over time.
/// This can be used to animate the color of a light source or mesh.
class PL_GAMEENGINE_DLL plColorAnimationComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plColorAnimationComponent, plComponent, plColorAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent
public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plColorAnimationComponent
public:
  plColorAnimationComponent();

  /// \brief How long it takes to sample the entire color gradient.
  plTime m_Duration; // [ property ]

  void SetColorGradientFile(const char* szFile); // [ property ]
  const char* GetColorGradientFile() const;      // [ property ]

  void SetColorGradient(const plColorGradientResourceHandle& hResource);
  PL_ALWAYS_INLINE const plColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  /// \brief How the animation should be played and looped.
  plEnum<plPropertyAnimMode> m_AnimationMode; // [ property ]

  /// \brief How the color should be applied to the target.
  plEnum<plSetColorMode> m_SetColorMode;      // [ property ]

  bool GetApplyRecursive() const;     // [ property ]
  void SetApplyRecursive(bool value); // [ property ]

  bool GetRandomStartOffset() const;     // [ property ]
  void SetRandomStartOffset(bool value); // [ property ]

protected:
  void Update();

  plTime m_CurAnimTime;
  plColorGradientResourceHandle m_hGradient;
};
