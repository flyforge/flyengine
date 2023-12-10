#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class QProgressDialog;
class QWinTaskbarProgress;
class QWinTaskbarButton;
class plProgress;
struct plProgressEvent;

/// \brief A Qt implementation to display the state of an plProgress instance.
///
/// Create a single instance of this at application startup and link it to an plProgress instance.
/// Whenever the instance's progress state changes, this class will display a simple progress bar.
class PLASMA_GUIFOUNDATION_DLL plQtProgressbar
{
public:
  plQtProgressbar();
  ~plQtProgressbar();

  /// \brief Sets the plProgress instance that should be visualized.
  void SetProgressbar(plProgress* pProgress);

  bool IsProcessingEvents() const { return m_iNestedProcessEvents > 0; }

private:
  void ProgressbarEventHandler(const plProgressEvent& e);

  void EnsureCreated();
  void EnsureDestroyed();

  QProgressDialog* m_pDialog = nullptr;
  plProgress* m_pProgress = nullptr;
  plInt32 m_iNestedProcessEvents = 0;

  QMetaObject::Connection m_OnDialogDestroyed;
};
