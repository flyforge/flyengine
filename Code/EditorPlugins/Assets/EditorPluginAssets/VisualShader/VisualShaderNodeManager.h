#pragma once

#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

struct plVisualShaderPinDescriptor;

class plVisualShaderPin : public plPin
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plVisualShaderPin, plPin);

public:
  plVisualShaderPin(Type type, const plVisualShaderPinDescriptor* pDescriptor, const plDocumentObject* pObject);

  const plRTTI* GetDataType() const;
  const plString& GetTooltip() const;
  const plVisualShaderPinDescriptor* GetDescriptor() const { return m_pDescriptor; }

private:
  const plVisualShaderPinDescriptor* m_pDescriptor;
};

class plVisualShaderNodeManager : public plDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const plDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const plDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const override;

  virtual plStatus InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_result) const override;
  virtual plStringView GetTypeCategory(const plRTTI* pRtti) const override;

private:
  virtual plStatus InternalCanAdd(
    const plRTTI* pRtti, const plDocumentObject* pParent, plStringView sParentProperty, const plVariant& index) const override;

  plUInt32 CountNodesOfType(plVisualShaderNodeType::Enum type) const;
};
