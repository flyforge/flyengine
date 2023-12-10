#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtColorGradientEditorWidget;

class plQtColorGradientAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtColorGradientAssetDocumentWindow(plDocument* pDocument);
  ~plQtColorGradientAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "ColorGradientAsset"; }

private Q_SLOTS:
  void onGradientColorCpAdded(double posX, const plColorGammaUB& color);
  void onGradientAlphaCpAdded(double posX, plUInt8 alpha);
  void onGradientIntensityCpAdded(double posX, float intensity);

  void MoveCP(plInt32 idx, double newPosX, const char* szArrayName);
  void onGradientColorCpMoved(plInt32 idx, double newPosX);
  void onGradientAlphaCpMoved(plInt32 idx, double newPosX);
  void onGradientIntensityCpMoved(plInt32 idx, double newPosX);

  void RemoveCP(plInt32 idx, const char* szArrayName);
  void onGradientColorCpDeleted(plInt32 idx);
  void onGradientAlphaCpDeleted(plInt32 idx);
  void onGradientIntensityCpDeleted(plInt32 idx);

  void onGradientColorCpChanged(plInt32 idx, const plColorGammaUB& color);
  void onGradientAlphaCpChanged(plInt32 idx, plUInt8 alpha);
  void onGradientIntensityCpChanged(plInt32 idx, float intensity);

  void onGradientBeginOperation();
  void onGradientEndOperation(bool commit);

  void onGradientNormalizeRange();

private:
  void UpdatePreview();

  void SendLiveResourcePreview();
  void RestoreResource();

  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const plDocumentObjectStructureEvent& e);

  bool m_bShowFirstTime;
  plQtColorGradientEditorWidget* m_pGradientEditor;
};

