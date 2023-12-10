#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Utils/Blackboard.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

namespace plRmlUiInternal
{
  BlackboardDataBinding::BlackboardDataBinding(const plSharedPtr<plBlackboard>& pBlackboard)
    : m_pBlackboard(pBlackboard)
  {
  }

  BlackboardDataBinding::~BlackboardDataBinding() = default;

  plResult BlackboardDataBinding::Initialize(Rml::Context& context)
  {
    if (m_pBlackboard == nullptr)
      return PLASMA_FAILURE;

    const char* szModelName = m_pBlackboard->GetName();
    if (plStringUtils::IsNullOrEmpty(szModelName))
    {
      plLog::Error("Can't bind a blackboard without a valid name");
      return PLASMA_FAILURE;
    }

    Rml::DataModelConstructor constructor = context.CreateDataModel(szModelName);
    if (!constructor)
    {
      return PLASMA_FAILURE;
    }

    for (auto it : m_pBlackboard->GetAllEntries())
    {
      m_EntryWrappers.emplace_back(*m_pBlackboard, it.Key(), it.Value().m_uiChangeCounter);
    }

    for (auto& wrapper : m_EntryWrappers)
    {
      constructor.BindFunc(
        wrapper.m_sName.GetData(),
        [&](Rml::Variant& out_Value) { wrapper.GetValue(out_Value); },
        [&](const Rml::Variant& value) { wrapper.SetValue(value); });
    }

    m_hDataModel = constructor.GetModelHandle();

    m_uiBlackboardChangeCounter = m_pBlackboard->GetBlackboardChangeCounter();
    m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();

    return PLASMA_SUCCESS;
  }

  void BlackboardDataBinding::Deinitialize(Rml::Context& context)
  {
    if (m_pBlackboard != nullptr)
    {
      context.RemoveDataModel(m_pBlackboard->GetName());
    }
  }

  void BlackboardDataBinding::Update()
  {
    if (m_uiBlackboardChangeCounter != m_pBlackboard->GetBlackboardChangeCounter())
    {
      plLog::Warning("Data Binding doesn't work with values that are registered or unregistered after setup");
      m_uiBlackboardChangeCounter = m_pBlackboard->GetBlackboardChangeCounter();
    }

    if (m_uiBlackboardEntryChangeCounter != m_pBlackboard->GetBlackboardEntryChangeCounter())
    {
      for (auto& wrapper : m_EntryWrappers)
      {
        auto pEntry = m_pBlackboard->GetEntry(wrapper.m_sName);

        if (pEntry != nullptr && wrapper.m_uiChangeCounter != pEntry->m_uiChangeCounter)
        {
          m_hDataModel.DirtyVariable(wrapper.m_sName.GetData());
          wrapper.m_uiChangeCounter = pEntry->m_uiChangeCounter;
        }
      }

      m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();
    }
  }

  //////////////////////////////////////////////////////////////////////////

  void BlackboardDataBinding::EntryWrapper::SetValue(const Rml::Variant& value)
  {
    plVariant::Type::Enum targetType = plVariant::Type::Invalid;
    if (auto pEntry = m_Blackboard.GetEntry(m_sName))
    {
      targetType = pEntry->m_Value.GetType();
    }

    if (m_Blackboard.SetEntryValue(m_sName, plRmlUiConversionUtils::ToVariant(value, targetType)).Failed())
    {
      plLog::Error("RmlUI: Can't set blackboard entry '{}', because it doesn't exist.", m_sName);
    }
  }

  void BlackboardDataBinding::EntryWrapper::GetValue(Rml::Variant& out_Value) const
  {
    out_Value = plRmlUiConversionUtils::ToVariant(m_Blackboard.GetEntryValue(m_sName));
  }

} // namespace plRmlUiInternal
