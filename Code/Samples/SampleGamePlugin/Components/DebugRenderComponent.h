#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct plMsgSetColor;

using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;

// Bitmask to allow the user to select what debug rendering the component should do
struct DebugRenderComponentMask
{
  using StorageType = plUInt8;

  // the enum names for the bits
  enum Enum
  {
    Box = PLASMA_BIT(0),
    Sphere = PLASMA_BIT(1),
    Cross = PLASMA_BIT(2),
    Quad = PLASMA_BIT(3),
    All = 0xFF,

    // required enum member; used by plBitflags for default initialization
    Default = All
  };

  // this allows the debugger to show us names for a bitmask
  // just try this out by looking at an plBitflags variable in a debugger
  struct Bits
  {
    plUInt8 Box : 1;
    plUInt8 Sphere : 1;
    plUInt8 Cross : 1;
    plUInt8 Quad : 1;
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_SAMPLEGAMEPLUGIN_DLL, DebugRenderComponentMask);

// use plComponentUpdateType::Always for this component to use 'Update' called even inside the editor when it is not simulating
// otherwise we would see the debug render output only when simulating the scene
using DebugRenderComponentManager = plComponentManagerSimple<class DebugRenderComponent, plComponentUpdateType::Always>;

class PLASMA_SAMPLEGAMEPLUGIN_DLL DebugRenderComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(DebugRenderComponent, plComponent, DebugRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // DebugRenderComponent

public:
  DebugRenderComponent();
  ~DebugRenderComponent();

  float m_fSize = 1.0f;             // [ property ]
  plColor m_Color = plColor::White; // [ property ]

  void SetTexture(const plTexture2DResourceHandle& hTexture);
  const plTexture2DResourceHandle& GetTexture() const;

  void SetTextureFile(const char* szFile); // [ property ]
  const char* GetTextureFile(void) const;  // [ property ]

  plBitflags<DebugRenderComponentMask> m_RenderTypes; // [ property ]

  void OnSetColor(plMsgSetColor& msg); // [ msg handler ]

  void SetRandomColor(); // [ scriptable ]

private:
  void Update();

  plTexture2DResourceHandle m_hTexture;
};
