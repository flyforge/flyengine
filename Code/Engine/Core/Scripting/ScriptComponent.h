#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Foundation/Types/RangeView.h>

using plScriptComponentManager = plComponentManager<class plScriptComponent, plBlockStorageType::FreeList>;

class PLASMA_CORE_DLL plScriptComponent : public plEventMessageHandlerComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plScriptComponent, plEventMessageHandlerComponent, plScriptComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

protected:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;
  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plScriptComponent
public:
  plScriptComponent();
  ~plScriptComponent();

  bool SendEventMessage(plMessage& inout_msg);
  void PostEventMessage(plMessage& inout_msg, plTime delay);

  void SetScriptClass(const plScriptClassResourceHandle& hScript);
  const plScriptClassResourceHandle& GetScriptClass() const { return m_hScriptClass; }

  void SetScriptClassFile(const char* szFile); // [ property ]
  const char* GetScriptClassFile() const;      // [ property ]

  void SetUpdateInterval(plTime interval); // [ property ]
  plTime GetUpdateInterval() const;        // [ property ]

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
  const plRangeView<const char*, plUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const plVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, plVariant& out_value) const;

  PLASMA_ALWAYS_INLINE plScriptInstance* GetScriptInstance() { return m_pInstance.Borrow(); }

private:
  void InstantiateScript(bool bActivate);
  void ClearInstance(bool bDeactivate);
  void UpdateScheduling();

  const plAbstractFunctionProperty* GetScriptFunction(plUInt32 uiFunctionIndex);
  void CallScriptFunction(plUInt32 uiFunctionIndex);

  void ReloadScript();

  plEventMessageSender<plMessage>& FindSender(plMessage& inout_msg);

  struct EventSender
  {
    const plRTTI* m_pMsgType = nullptr;
    plEventMessageSender<plMessage> m_Sender;
  };

  plHybridArray<EventSender, 2> m_EventSenders;

  plArrayMap<plHashedString, plVariant> m_Parameters;

  plScriptClassResourceHandle m_hScriptClass;
  plTime m_UpdateInterval = plTime::MakeZero();

  plSharedPtr<plScriptRTTI> m_pScriptType;
  plUniquePtr<plScriptInstance> m_pInstance;
};