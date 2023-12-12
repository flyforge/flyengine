#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class plProcGenPin : public plPin
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGenPin, plPin);

public:
  using plPin::plPin;
};

class plProcGenNodeManager : public plDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const plDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const plDocumentObject* pObject, NodeInternal& node) override;
  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& Types) const override;
  virtual const char* GetTypeCategory(const plRTTI* pRtti) const override;

  virtual plStatus InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_Result) const override;
};
