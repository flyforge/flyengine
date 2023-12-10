#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <RmlUiPlugin/RmlUiDataBinding.h>

class plBlackboard;

namespace plRmlUiInternal
{
  class BlackboardDataBinding final : public plRmlUiDataBinding
  {
  public:
    BlackboardDataBinding(const plSharedPtr<plBlackboard>& pBlackboard);
    ~BlackboardDataBinding();

    virtual plResult Initialize(Rml::Context& context) override;
    virtual void Deinitialize(Rml::Context& context) override;
    virtual void Update() override;

  private:
    plSharedPtr<plBlackboard> m_pBlackboard;

    Rml::DataModelHandle m_hDataModel;

    struct EntryWrapper
    {
      EntryWrapper(plBlackboard& blackboard, const plHashedString& sName, plUInt32 uiChangeCounter)
        : m_Blackboard(blackboard)
        , m_sName(sName)
        , m_uiChangeCounter(uiChangeCounter)
      {
      }

      void SetValue(const Rml::Variant& value);
      void GetValue(Rml::Variant& out_Value) const;

      plBlackboard& m_Blackboard;
      plHashedString m_sName;
      plUInt32 m_uiChangeCounter;
    };

    Rml::Vector<EntryWrapper> m_EntryWrappers;

    plUInt32 m_uiBlackboardChangeCounter = 0;
    plUInt32 m_uiBlackboardEntryChangeCounter = 0;
  };
} // namespace plRmlUiInternal
