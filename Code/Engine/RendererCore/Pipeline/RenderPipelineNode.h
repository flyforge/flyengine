#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

class plRenderPipelineNode;

struct plRenderPipelineNodePin
{
  PLASMA_DECLARE_POD_TYPE();

  struct Type
  {
    typedef plUInt8 StorageType;

    enum Enum
    {
      Unknown,
      Input,
      Output,
      PassThrough,

      Default = Unknown
    };
  };

  plEnum<Type> m_Type;
  plUInt8 m_uiInputIndex = 0xFF;
  plUInt8 m_uiOutputIndex = 0xFF;
  plRenderPipelineNode* m_pParent = nullptr;
};

struct plRenderPipelineNodeInputPin : public plRenderPipelineNodePin
{
  PLASMA_DECLARE_POD_TYPE();

  PLASMA_ALWAYS_INLINE plRenderPipelineNodeInputPin() { m_Type = Type::Input; }
};

struct plRenderPipelineNodeOutputPin : public plRenderPipelineNodePin
{
  PLASMA_DECLARE_POD_TYPE();

  PLASMA_ALWAYS_INLINE plRenderPipelineNodeOutputPin() { m_Type = Type::Output; }
};

struct plRenderPipelineNodePassThrougPin : public plRenderPipelineNodePin
{
  PLASMA_DECLARE_POD_TYPE();

  PLASMA_ALWAYS_INLINE plRenderPipelineNodePassThrougPin() { m_Type = Type::PassThrough; }
};

class PLASMA_RENDERERCORE_DLL plRenderPipelineNode : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRenderPipelineNode, plReflectedClass);

public:
  virtual ~plRenderPipelineNode() {}

  void InitializePins();

  plHashedString GetPinName(const plRenderPipelineNodePin* pPin) const;
  const plRenderPipelineNodePin* GetPinByName(const char* szName) const;
  const plRenderPipelineNodePin* GetPinByName(plHashedString sName) const;
  const plArrayPtr<const plRenderPipelineNodePin* const> GetInputPins() const { return m_InputPins; }
  const plArrayPtr<const plRenderPipelineNodePin* const> GetOutputPins() const { return m_OutputPins; }

private:
  plDynamicArray<const plRenderPipelineNodePin*> m_InputPins;
  plDynamicArray<const plRenderPipelineNodePin*> m_OutputPins;
  plHashTable<plHashedString, const plRenderPipelineNodePin*> m_NameToPin;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plRenderPipelineNodePin);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plRenderPipelineNodeInputPin);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plRenderPipelineNodeOutputPin);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plRenderPipelineNodePassThrougPin);
