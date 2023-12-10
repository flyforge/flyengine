#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

typedef plComponentManagerSimple<class plColorAnimationComponent, plComponentUpdateType::WhenSimulating> plColorAnimationComponentManager;

/// \brief Samples a color gradient and sends an plMsgSetColor to the object it is attached to
///
/// The color gradient is samples linearly over time. This can be used to animate the color of a light source or mesh.
/// \todo Expose the plSetColorMode of the plMsgSetColor
/// \todo Add speed parameter
/// \todo Add loop mode (once, back-and-forth, loop)
/// \todo Add option to send message to whole sub-tree (SendMessageRecursive)
/// \todo Add on-finished (loop point) event
class PLASMA_GAMEENGINE_DLL plColorAnimationComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plColorAnimationComponent, plComponent, plColorAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent
public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plColorAnimationComponent
public:
  plColorAnimationComponent();

  plTime m_Duration; // [ property ]

  void SetColorGradientFile(const char* szFile); // [ property ]
  const char* GetColorGradientFile() const;      // [ property ]

  void SetColorGradient(const plColorGradientResourceHandle& hResource);
  PLASMA_ALWAYS_INLINE const plColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  plEnum<plPropertyAnimMode> m_AnimationMode; // [ property ]
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
