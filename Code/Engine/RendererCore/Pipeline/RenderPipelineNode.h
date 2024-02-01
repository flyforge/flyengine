#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

class plRenderPipelineNode;

struct plRenderPipelineNodePin
{
  PL_DECLARE_POD_TYPE();

  struct Type
  {
    using StorageType = plUInt8;

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
  PL_DECLARE_POD_TYPE();

  PL_ALWAYS_INLINE plRenderPipelineNodeInputPin() { m_Type = Type::Input; }
};

struct plRenderPipelineNodeOutputPin : public plRenderPipelineNodePin
{
  PL_DECLARE_POD_TYPE();

  PL_ALWAYS_INLINE plRenderPipelineNodeOutputPin() { m_Type = Type::Output; }
};

struct plRenderPipelineNodePassThrougPin : public plRenderPipelineNodePin
{
  PL_DECLARE_POD_TYPE();

  PL_ALWAYS_INLINE plRenderPipelineNodePassThrougPin() { m_Type = Type::PassThrough; }
};

class PL_RENDERERCORE_DLL plRenderPipelineNode : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plRenderPipelineNode, plReflectedClass);

public:
  virtual ~plRenderPipelineNode() = default;

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

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plRenderPipelineNodePin);
PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plRenderPipelineNodeInputPin);
PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plRenderPipelineNodeOutputPin);
PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plRenderPipelineNodePassThrougPin);
