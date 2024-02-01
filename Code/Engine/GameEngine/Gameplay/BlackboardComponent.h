#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/Blackboard.h>
#include <GameEngine/GameEngineDLL.h>

struct plMsgUpdateLocalBounds;
struct plMsgExtractRenderData;

using plBlackboardTemplateResourceHandle = plTypedResourceHandle<class plBlackboardTemplateResource>;

struct plBlackboardEntry
{
  plHashedString m_sName;
  plVariant m_InitialValue;
  plBitflags<plBlackboardEntryFlags> m_Flags;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plBlackboardEntry);

//////////////////////////////////////////////////////////////////////////

struct PL_GAMEENGINE_DLL plMsgBlackboardEntryChanged : public plEventMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgBlackboardEntryChanged, plEventMessage);

  plHashedString m_sName;
  plVariant m_OldValue;
  plVariant m_NewValue;

private:
  const char* GetName() const { return m_sName; }
  void SetName(const char* szName) { m_sName.Assign(szName); }
};

//////////////////////////////////////////////////////////////////////////

/// \brief This base component represents an plBlackboard, which can be used to share state between multiple components and objects.
///
/// The derived implementations may either create their own blackboards or reference other blackboards.
class PL_GAMEENGINE_DLL plBlackboardComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plBlackboardComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plBlackboardComponent

public:
  plBlackboardComponent();
  ~plBlackboardComponent();

  /// \brief Try to find a plBlackboardComponent on pSearchObject or its parents with the given name and returns its blackboard.
  ///
  /// The blackboard name is only checked if the given name is not empty. If no matching blackboard component is found,
  /// the function will call plBlackboard::GetOrCreateGlobal() with the given name. Thus if you provide a name, you will always get a result, either from a component or from the global storage.
  ///
  /// \sa plBlackboard::GetOrCreateGlobal()
  static plSharedPtr<plBlackboard> FindBlackboard(plGameObject* pSearchObject, plStringView sBlackboardName = plStringView());

  /// \brief Returns the blackboard owned by this component
  const plSharedPtr<plBlackboard>& GetBoard();
  plSharedPtr<const plBlackboard> GetBoard() const;

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  void SetTemplateFile(const char* szName); // [ property ]
  const char* GetTemplateFile() const;      // [ property ]

  void SetEntryValue(const char* szName, const plVariant& value); // [ scriptable ]
  plVariant GetEntryValue(const char* szName) const;              // [ scriptable ]

protected:
  static plBlackboard* Reflection_FindBlackboard(plGameObject* pSearchObject, plStringView sBlackboardName);

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(plMsgExtractRenderData& msg) const;

  plSharedPtr<plBlackboard> m_pBoard;

  plBlackboardTemplateResourceHandle m_hTemplate;
};

//////////////////////////////////////////////////////////////////////////

using plLocalBlackboardComponentManager = plComponentManager<class plLocalBlackboardComponent, plBlockStorageType::Compact>;

/// \brief This component creates its own plBlackboard, and thus locally holds state.
class PL_GAMEENGINE_DLL plLocalBlackboardComponent : public plBlackboardComponent
{
  PL_DECLARE_COMPONENT_TYPE(plLocalBlackboardComponent, plBlackboardComponent, plLocalBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plBlackboardComponent

public:
  plLocalBlackboardComponent();
  plLocalBlackboardComponent(plLocalBlackboardComponent&& other);
  ~plLocalBlackboardComponent();

  plLocalBlackboardComponent& operator=(plLocalBlackboardComponent&& other);

  void SetSendEntryChangedMessage(bool bSend); // [ property ]
  bool GetSendEntryChangedMessage() const;     // [ property ]

  void SetBlackboardName(const char* szName); // [ property ]
  const char* GetBlackboardName() const;      // [ property ]

private:
  plUInt32 Entries_GetCount() const;
  const plBlackboardEntry& Entries_GetValue(plUInt32 uiIndex) const;
  void Entries_SetValue(plUInt32 uiIndex, const plBlackboardEntry& entry);
  void Entries_Insert(plUInt32 uiIndex, const plBlackboardEntry& entry);
  void Entries_Remove(plUInt32 uiIndex);

  void OnEntryChanged(const plBlackboard::EntryEvent& e);
  void InitializeFromTemplate();

  // this array is not held during runtime, it is only needed during editor time until the component is serialized out
  plDynamicArray<plBlackboardEntry> m_InitialEntries;

  plEventMessageSender<plMsgBlackboardEntryChanged> m_EntryChangedSender; // [ event ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct plGlobalBlackboardInitMode
{
  using StorageType = plUInt8;

  enum Enum : StorageType
  {
    EnsureEntriesExist,    ///< Brief only adds entries to the blackboard, that haven't been added before. Doesn't change the values of existing entries.
    ResetEntryValues,      ///< Overwrites values of existing entries, to reset them to the start value defined in the template.
    ClearEntireBlackboard, ///< Removes all entries from the blackboard and only adds the ones from the template. This also gets rid of temporary values.

    Default = ClearEntireBlackboard
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plGlobalBlackboardInitMode);

using plGlobalBlackboardComponentManager = plComponentManager<class plGlobalBlackboardComponent, plBlockStorageType::Compact>;

/// \brief This component references a global blackboard by name. If necessary, the blackboard will be created.
///
/// This allows to initialize a global blackboard with known values.
class PL_GAMEENGINE_DLL plGlobalBlackboardComponent : public plBlackboardComponent
{
  PL_DECLARE_COMPONENT_TYPE(plGlobalBlackboardComponent, plBlackboardComponent, plGlobalBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plGlobalBlackboardComponent

public:
  plGlobalBlackboardComponent();
  plGlobalBlackboardComponent(plGlobalBlackboardComponent&& other);
  ~plGlobalBlackboardComponent();

  plGlobalBlackboardComponent& operator=(plGlobalBlackboardComponent&& other);

  void SetBlackboardName(const char* szName); // [ property ]
  const char* GetBlackboardName() const;      // [ property ]

  plEnum<plGlobalBlackboardInitMode> m_InitMode; // [ property ]

private:
  void InitializeFromTemplate();

  plHashedString m_sName;
};
