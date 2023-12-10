#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/RefCounted.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

/// \brief Describes the current meta state of a property for display purposes in the property grid
struct plPropertyUiState
{
  enum Visibility
  {
    Default,   ///< Displayed normally, for editing (unless the property is read-only)
    Invisible, ///< Hides the property entirely
    Disabled,  ///< The property is shown but disabled, when multiple objects are selected and in one the property is invisible, in the other it is
               ///< disabled, the disabled state takes precedence
  };

  plPropertyUiState()
  {
    m_Visibility = Visibility::Default;
  }

  Visibility m_Visibility;
  plString m_sNewLabelText;
};

/// \brief Event that is broadcast whenever information about how to present properties is required
struct plPropertyMetaStateEvent
{
  /// The object for which the information is queried
  const plDocumentObject* m_pObject = nullptr;

  /// The map into which event handlers should write their information about the state of each property.
  /// The string is the property name that identifies the property in m_pObject.
  plMap<plString, plPropertyUiState>* m_pPropertyStates = nullptr;
};

/// \brief Event that is broadcast whenever information about how to present elements in a container is required
struct plContainerElementMetaStateEvent
{
  /// The object for which the information is queried
  const plDocumentObject* m_pObject = nullptr;
  /// The Container property
  const char* m_szProperty = nullptr;
  /// The map into which event handlers should write their information about the state of each container element.
  /// The plVariant should be the key of the container element, either plUInt32 for arrays and sets or plString for maps.
  plHashTable<plVariant, plPropertyUiState>* m_pContainerElementStates = nullptr;
};

/// \brief This class allows to query additional information about how to present properties in the property grid
///
/// The property grid calls GetTypePropertiesState() and GetContainerElementsState() with the current selection of plDocumentObject's.
/// This triggers the plPropertyMetaStateEvent to be broadcast, which allows for other code to determine additional
/// information for the properties and write it into the event data.
class PLASMA_GUIFOUNDATION_DLL plPropertyMetaState
{
  PLASMA_DECLARE_SINGLETON(plPropertyMetaState);

public:
  plPropertyMetaState();

  /// \brief Queries the property meta state for a single plDocumentObject
  void GetTypePropertiesState(const plDocumentObject* pObject, plMap<plString, plPropertyUiState>& out_propertyStates);

  /// \brief Queries the property meta state for a multi selection of plDocumentObject's
  ///
  /// This will query the information for every single selected object and then merge the result into one.
  void GetTypePropertiesState(const plHybridArray<plPropertySelection, 8>& items, plMap<plString, plPropertyUiState>& out_propertyStates);

  /// \brief Queries the meta state for the elements of a single container property on one plDocumentObject.
  void GetContainerElementsState(const plDocumentObject* pObject, const char* szProperty, plHashTable<plVariant, plPropertyUiState>& out_propertyStates);

  /// \brief Queries the meta state for the elements of a single container property on a multi selection of plDocumentObjects.
  ///
  /// This will query the information for every single selected object and then merge the result into one.
  void GetContainerElementsState(const plHybridArray<plPropertySelection, 8>& items, const char* szProperty, plHashTable<plVariant, plPropertyUiState>& out_propertyStates);

  /// Attach to this event to get notified of property state queries.
  /// Add information to plPropertyMetaStateEvent::m_pPropertyStates to return data.
  plEvent<plPropertyMetaStateEvent&> m_Events;
  /// Attach to this event to get notified of container element state queries.
  /// Add information to plContainerElementMetaStateEvent::m_pContainerElementStates to return data.
  plEvent<plContainerElementMetaStateEvent&> m_ContainerEvents;

private:
  plMap<plString, plPropertyUiState> m_Temp;
  plHashTable<plVariant, plPropertyUiState> m_Temp2;
};
