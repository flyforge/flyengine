#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class plVisualizerAttribute;
class plDocumentObject;
struct plDocumentObjectPropertyEvent;
struct plQtDocumentWindowEvent;
class plObjectAccessorBase;

/// \brief Base class for the editor side code that sets up a 'visualizer' for object properties.
///
/// Typically visualizers are configured with plVisualizerAttribute's on component types.
/// The adapter reads the attribute values and sets up the necessary code to render them in the engine.
/// This is usually achieved by creating plEngineGizmoHandle objects (which get automatically synchronized
/// with the engine process).
/// The adapter then reacts to editor side object changes and adjusts the engine side representation
/// as needed.
class PLASMA_EDITORFRAMEWORK_DLL plVisualizerAdapter
{
public:
  plVisualizerAdapter();
  virtual ~plVisualizerAdapter();

  void SetVisualizer(const plVisualizerAttribute* pAttribute, const plDocumentObject* pObject);

private:
  void DocumentObjectPropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const plQtDocumentWindowEvent& e);
  void DocumentObjectMetaDataEventHandler(const plObjectMetaData<plUuid, plDocumentObjectMetaData>::EventData& e);

protected:
  virtual plTransform GetObjectTransform() const;
  plObjectAccessorBase* GetObjectAccessor() const;
  const plAbstractProperty* GetProperty(const char* szProperty) const;

  /// \brief Called to actually properly set up the adapter. All setup code is implemented here.
  virtual void Finalize() = 0;
  /// \brief Called when object properties have changed and the visualizer may need to react.
  virtual void Update() = 0;
  /// \brief Called when the object has been moved somehow. More light weight than a full update.
  virtual void UpdateGizmoTransform() = 0;

  bool m_bVisualizerIsVisible;
  const plVisualizerAttribute* m_pVisualizerAttr;
  const plDocumentObject* m_pObject;
};
