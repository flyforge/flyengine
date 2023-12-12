#pragma once

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/DuktapeContext.h>
#include <Core/World/Component.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

class plTypeScriptBinding;

struct PLASMA_TYPESCRIPTPLUGIN_DLL plMsgTypeScriptMsgProxy : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgTypeScriptMsgProxy, plMessage);

  plUInt32 m_uiTypeNameHash = 0;
  plUInt32 m_uiStashIndex = 0;
};

class PLASMA_TYPESCRIPTPLUGIN_DLL plTypeScriptComponentManager : public plComponentManager<class plTypeScriptComponent, plBlockStorageType::FreeList>
{
  using SUPER = plComponentManager<class plTypeScriptComponent, plBlockStorageType::FreeList>;

public:
  plTypeScriptComponentManager(plWorld* pWorld);
  ~plTypeScriptComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  plTypeScriptBinding& GetTsBinding() const { return m_TsBinding; }

private:
  void Update(const plWorldModule::UpdateContext& context);

  mutable plTypeScriptBinding m_TsBinding;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_TYPESCRIPTPLUGIN_DLL plTypeScriptComponent : public plEventMessageHandlerComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plTypeScriptComponent, plEventMessageHandlerComponent, plTypeScriptComponentManager);

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

  virtual bool HandlesMessage(const plMessage& msg) const override;
  virtual bool OnUnhandledMessage(plMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(plMessage& msg, bool bWasPostedMsg) const override;

  bool HandleUnhandledMessage(plMessage& msg, bool bWasPostedMsg);

  //////////////////////////////////////////////////////////////////////////
  // plTypeScriptComponent

public:
  plTypeScriptComponent();
  ~plTypeScriptComponent();

  void BroadcastEventMsg(plEventMessage& msg);

  void SetUpdateInterval(plTime interval) { m_UpdateInterval = interval; }

  void SetTypeScriptComponentGuid(const plUuid& hResource);
  const plUuid& GetTypeScriptComponentGuid() const;

private:
  struct EventSender
  {
    const plRTTI* m_pMsgType = nullptr;
    plEventMessageSender<plEventMessage> m_Sender;
  };

  plHybridArray<EventSender, 2> m_EventSenders;

  bool CallTsFunc(const char* szFuncName);
  void Update(plTypeScriptBinding& script);
  void SetExposedVariables();

  plTypeScriptBinding::TsComponentTypeInfo m_ComponentTypeInfo;

  void SetTypeScriptComponentFile(const char* szFile); // [ property ]
  const char* GetTypeScriptComponentFile() const;      // [ property ]

  void OnMsgTypeScriptMsgProxy(plMsgTypeScriptMsgProxy& msg); // [ message handler ]

  enum UserFlag
  {
    InitializedTS = 0,
    OnActivatedTS = 1,
    NoTsTick = 2,
    SimStartedTS = 3,
    ScriptFailure = 4,
  };

private:
  plUuid m_TypeScriptComponentGuid;
  plTime m_LastUpdate;
  plTime m_UpdateInterval = plTime::Zero();

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters

public:
  const plRangeView<const char*, plUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const plVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, plVariant& out_value) const;

private:
  plArrayMap<plHashedString, plVariant> m_Parameters;
};
