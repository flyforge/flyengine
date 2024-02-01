#pragma once

#include <Foundation/Strings/HashedString.h>

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>

class plRmlUiContext;

namespace plRmlUiInternal
{
  class EventListener final : public Rml::EventListener
  {
  public:
    virtual void ProcessEvent(Rml::Event& ref_event) override;

    virtual void OnDetach(Rml::Element* pElement) override;

  private:
    friend class EventListenerInstancer;
    plHashedString m_sIdentifier;
    plUInt32 m_uiIndex = 0;
  };

  class EventListenerInstancer final : public Rml::EventListenerInstancer
  {
  public:
    EventListenerInstancer();
    ~EventListenerInstancer();

    virtual Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* pElement) override;

    void ReturnToPool(EventListener& ref_listener);

  private:
    plDeque<EventListener> m_EventListenerPool;
    plDynamicArray<plUInt32> m_EventListenerFreelist;
  };
} // namespace plRmlUiInternal
