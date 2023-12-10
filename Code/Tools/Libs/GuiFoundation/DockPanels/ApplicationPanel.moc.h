#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ads/DockWidget.h>

class plQtContainerWindow;

/// \brief Base class for all panels that are supposed to be application wide (not tied to some document).
class PLASMA_GUIFOUNDATION_DLL plQtApplicationPanel : public ads::CDockWidget
{
public:
  Q_OBJECT

public:
  plQtApplicationPanel(const char* szPanelName);
  ~plQtApplicationPanel();

  void EnsureVisible();

  static const plDynamicArray<plQtApplicationPanel*>& GetAllApplicationPanels() { return s_AllApplicationPanels; }

protected:
  virtual void ToolsProjectEventHandler(const plToolsProjectEvent& e);
  virtual bool event(QEvent* event) override;

private:
  friend class plQtContainerWindow;

  static plDynamicArray<plQtApplicationPanel*> s_AllApplicationPanels;

  plQtContainerWindow* m_pContainerWindow;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GUIFOUNDATION_DLL, plQtApplicationPanel);

