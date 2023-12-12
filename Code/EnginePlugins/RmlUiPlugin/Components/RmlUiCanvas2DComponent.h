#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RmlUiPlugin/Components/RmlUiMessages.h>

struct plMsgExtractRenderData;
class plRmlUiContext;
class plRmlUiDataBinding;
class plBlackboard;

using plRmlUiResourceHandle = plTypedResourceHandle<class plRmlUiResource>;

using plRmlUiCanvas2DComponentManager = plComponentManagerSimple<class plRmlUiCanvas2DComponent, plComponentUpdateType::Always, plBlockStorageType::Compact>;

class PLASMA_RMLUIPLUGIN_DLL plRmlUiCanvas2DComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plRmlUiCanvas2DComponent, plRenderComponent, plRmlUiCanvas2DComponentManager);

public:
  plRmlUiCanvas2DComponent();
  ~plRmlUiCanvas2DComponent();

  plRmlUiCanvas2DComponent& operator=(plRmlUiCanvas2DComponent&& rhs);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void Update();

  void SetRmlFile(const char* szFile); // [ property ]
  const char* GetRmlFile() const;      // [ property ]

  void SetRmlResource(const plRmlUiResourceHandle& hResource);
  const plRmlUiResourceHandle& GetRmlResource() const { return m_hResource; }

  void SetOffset(const plVec2I32& offset);                // [ property ]
  const plVec2I32& GetOffset() const { return m_vOffset; } // [ property ]

  void SetSize(const plVec2U32& size);                // [ property ]
  const plVec2U32& GetSize() const { return m_vSize; } // [ property ]

  void SetAnchorPoint(const plVec2& anchorPoint);                // [ property ]
  const plVec2& GetAnchorPoint() const { return m_vAnchorPoint; } // [ property ]

  void SetPassInput(bool bPassInput);                // [ property ]
  bool GetPassInput() const { return m_bPassInput; } // [ property ]

  /// \brief Look for a blackboard component on the owner object and its parent and bind their blackboards during initialization of this component.
  void SetAutobindBlackboards(bool bAutobind);                           // [ property ]
  bool GetAutobindBlackboards() const { return m_bAutobindBlackboards; } // [ property ]

  plUInt32 AddDataBinding(plUniquePtr<plRmlUiDataBinding>&& dataBinding);
  void RemoveDataBinding(plUInt32 uiDataBindingIndex);

  /// \brief Adds the given blackboard as data binding. The name of the board is used as model name for the binding.
  plUInt32 AddBlackboardBinding(const plSharedPtr<plBlackboard>& pBlackboard);
  void RemoveBlackboardBinding(plUInt32 uiDataBindingIndex);

  plRmlUiContext* GetOrCreateRmlContext();
  plRmlUiContext* GetRmlContext() { return m_pContext; }

  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void OnMsgReload(plMsgRmlUiReload& msg);
  void UpdateCachedValues();
  void UpdateAutobinding();

  plRmlUiResourceHandle m_hResource;
  plEvent<const plResourceEvent&, plMutex>::Unsubscriber m_ResourceEventUnsubscriber;

  plVec2I32 m_vOffset = plVec2I32::ZeroVector();
  plVec2U32 m_vSize = plVec2U32::ZeroVector();
  plVec2 m_vAnchorPoint = plVec2::ZeroVector();
  plVec2U32 m_vReferenceResolution = plVec2U32::ZeroVector();
  bool m_bPassInput = true;
  bool m_bAutobindBlackboards = true;

  plRmlUiContext* m_pContext = nullptr;

  plDynamicArray<plUniquePtr<plRmlUiDataBinding>> m_DataBindings;
  plDynamicArray<plUInt32> m_AutoBindings;
};
