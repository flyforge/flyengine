#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Communication/Event.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>

class plImageDataAssetDocument;
struct plImageDataAssetEvent;

class plQtImageDataAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtImageDataAssetDocumentWindow(plImageDataAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "ImageDataAsset"; }

private:
  void ImageDataAssetEventHandler(const plImageDataAssetEvent& e);
  plEvent<const plImageDataAssetEvent&>::Unsubscriber m_EventUnsubscriper;

  void UpdatePreview();

  QPointer<plQtImageWidget> m_pImageWidget;
};
