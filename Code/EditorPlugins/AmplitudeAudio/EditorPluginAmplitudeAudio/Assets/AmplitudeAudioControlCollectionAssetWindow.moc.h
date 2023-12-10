#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtAmplitudeAudioControlCollectionAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtAmplitudeAudioControlCollectionAssetDocumentWindow(plDocument* pDocument);
  ~plQtAmplitudeAudioControlCollectionAssetDocumentWindow();

  const char* GetWindowLayoutGroupName() const override { return "AudioControlCollectionAsset"; }
};
