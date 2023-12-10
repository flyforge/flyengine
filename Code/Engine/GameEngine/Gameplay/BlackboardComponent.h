#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/Blackboard.h>
#include <GameEngine/GameEngineDLL.h>

using plBlackboardTemplateResourceHandle = plTypedResourceHandle<class plBlackboardTemplateResource>;

struct plBlackboardEntry
{
  plHashedString m_sName;
  plVariant m_InitialValue;
  plBitflags<plBlackboardEntryFlags> m_Flags;

  void SetName(const char* szName) { m_sName.Assign(szName); }
  const char* GetName() const { return m_sName; }

  plResult Serialize(plStreamWriter& stream) const;
  plResult Deserialize(plStreamReader& stream);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plBlackboardEntry);

//////////////////////////////////////////////////////////////////////////

struct PLASMA_GAMEENGINE_DLL plMsgBlackboardEntryChanged : public plEventMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgBlackboardEntryChanged, plEventMessage);

  plHashedString m_sName;
  plVariant m_OldValue;
  plVariant m_NewValue;

private:
  const char* GetName() const { return m_sName; }
  void SetName(const char* szName) { m_sName.Assign(szName); }
};

//////////////////////////////////////////////////////////////////////////

struct plMsgUpdateLocalBounds;
struct plMsgExtractRenderData;

using plBlackboardComponentManager = plComponentManager<class plBlackboardComponent, plBlockStorageType::Compact>;

/// \brief This component holds an plBlackboard which can be used to share state between multiple components.
class PLASMA_GAMEENGINE_DLL plBlackboardComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plBlackboardComponent, plComponent, plBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plBlackboardComponent

public:
  plBlackboardComponent();
  plBlackboardComponent(plBlackboardComponent&& other);
  ~plBlackboardComponent();

  plBlackboardComponent& operator=(plBlackboardComponent&& other);

  /// \brief Try to find a plBlackboardComponent on pSearchObject or its parents with the given name and returns its blackboard.
  ///
  /// The blackboard name is only checked if the given name is not empty. If no matching blackboard component is found,
  /// the function will try to find a global blackboard with the given name.
  ///
  /// \sa plBlackboard::FindGlobal()
  static plSharedPtr<plBlackboard> FindBlackboard(plGameObject* pSearchObject, plStringView sBlackboardName = plStringView());

  /// \brief Returns the blackboard owned by this component
  const plSharedPtr<plBlackboard>& GetBoard();
  plSharedPtr<const plBlackboard> GetBoard() const;

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  void SetSendEntryChangedMessage(bool bSend); // [ property ]
  bool GetSendEntryChangedMessage() const;     // [ property ]

  void SetBlackboardName(const char* szName); // [ property ]
  const char* GetBlackboardName() const;      // [ property ]

  void SetEntryValue(const char* szName, const plVariant& value); // [ scriptable ]
  plVariant GetEntryValue(const char* szName) const;                    // [ scriptable ]

  void SetTemplateFile(const char* szName); // [ property ]
  const char* GetTemplateFile() const;      // [ property ]


private:
  plUInt32 Entries_GetCount() const;
  const plBlackboardEntry& Entries_GetValue(plUInt32 uiIndex) const;
  void Entries_SetValue(plUInt32 uiIndex, const plBlackboardEntry& entry);
  void Entries_Insert(plUInt32 uiIndex, const plBlackboardEntry& entry);
  void Entries_Remove(plUInt32 uiIndex);

  static plBlackboard* Reflection_FindBlackboard(plGameObject* pSearchObject, plStringView sBlackboardName);

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(plMsgExtractRenderData& msg) const;
  void OnEntryChanged(const plBlackboard::EntryEvent& e);
  void InitializeFromTemplate();

  plSharedPtr<plBlackboard> m_pBoard;

  // this array is not held during runtime, it is only needed during editor time until the component is serialized out
  plDynamicArray<plBlackboardEntry> m_InitialEntries;

  plEventMessageSender<plMsgBlackboardEntryChanged> m_EntryChangedSender; // [ event ]

  plBlackboardTemplateResourceHandle m_hTemplate;
};
