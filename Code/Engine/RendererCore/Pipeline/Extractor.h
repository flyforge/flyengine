#pragma once

#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/RenderData.h>

class PLASMA_RENDERERCORE_DLL plExtractor : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExtractor, plReflectedClass);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plExtractor);

public:
  plExtractor(const char* szName);
  virtual ~plExtractor();

  /// \brief Sets the name of the extractor.
  void SetName(const char* szName);

  /// \brief returns the name of the extractor.
  const char* GetName() const;

  virtual void Extract(const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData);

  virtual void PostSortAndBatch(const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData);

protected:
  /// \brief returns true if the given object should be filtered by view tags.
  bool FilterByViewTags(const plView& view, const plGameObject* pObject) const;

  /// \brief extracts the render data for the given object.
  void ExtractRenderData(const plView& view, const plGameObject* pObject, plMsgExtractRenderData& msg, plExtractedRenderData& extractedRenderData) const;

private:
  friend class plRenderPipeline;

  bool m_bActive;

  plHashedString m_sName;

protected:
  plHybridArray<plHashedString, 4> m_DependsOn;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  mutable plUInt32 m_uiNumCachedRenderData;
  mutable plUInt32 m_uiNumUncachedRenderData;
#endif
};


class PLASMA_RENDERERCORE_DLL plVisibleObjectsExtractor : public plExtractor
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plVisibleObjectsExtractor, plExtractor);

public:
  plVisibleObjectsExtractor(const char* szName = "VisibleObjectsExtractor");
  ~plVisibleObjectsExtractor();

  virtual void Extract(const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData) override;
};

class PLASMA_RENDERERCORE_DLL plSelectedObjectsExtractorBase : public plExtractor
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSelectedObjectsExtractorBase, plExtractor);

public:
  plSelectedObjectsExtractorBase(const char* szName = "SelectedObjectsExtractor");
  ~plSelectedObjectsExtractorBase();

  virtual void Extract(const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData) override;

  virtual const plDeque<plGameObjectHandle>* GetSelection() = 0;

  plRenderData::Category m_OverrideCategory;
};

/// \brief Stores a list of game objects that should get highlighted by the renderer.
///
/// Store an instance somewhere in your game code:
/// plSelectedObjectsContext m_SelectedObjects;
/// Add handles to game object that should be get the highlighting outline (as the editor uses for selected objects).
/// On an plView call:
/// plView::SetExtractorProperty("HighlightObjects", "SelectionContext", &m_SelectedObjects);
/// The first name must be the name of an plSelectedObjectsExtractor that is instantiated by the render pipeline.
///
/// As long as there is also an plSelectionHighlightPass in the render pipeline, all objects in this selection will be rendered
/// with an outline.
class PLASMA_RENDERERCORE_DLL plSelectedObjectsContext : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSelectedObjectsContext, plReflectedClass);

public:
  plSelectedObjectsContext();
  ~plSelectedObjectsContext();

  void RemoveDeadObjects(const plWorld& world);
  void AddObjectAndChildren(const plWorld& world, const plGameObjectHandle& hObject);
  void AddObjectAndChildren(const plWorld& world, const plGameObject* pObject);

  plDeque<plGameObjectHandle> m_Objects;
};

/// \brief An extractor that can be instantiated in a render pipeline, to define manually which objects should be rendered with a selection outline.
///
/// \sa plSelectedObjectsContext
class PLASMA_RENDERERCORE_DLL plSelectedObjectsExtractor : public plSelectedObjectsExtractorBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSelectedObjectsExtractor, plSelectedObjectsExtractorBase);

public:
  plSelectedObjectsExtractor(const char* szName = "ExplicitlySelectedObjectsExtractor");
  ~plSelectedObjectsExtractor();

  virtual const plDeque<plGameObjectHandle>* GetSelection() override;

  /// \brief The context is typically set through an plView, through plView::SetExtractorProperty("<name>", "SelectionContext", pointer);
  void SetSelectionContext(plSelectedObjectsContext* pSelectionContext) { m_pSelectionContext = pSelectionContext; } // [ property ]
  plSelectedObjectsContext* GetSelectionContext() const { return m_pSelectionContext; }                              // [ property ]

private:
  plSelectedObjectsContext* m_pSelectionContext = nullptr;
};
