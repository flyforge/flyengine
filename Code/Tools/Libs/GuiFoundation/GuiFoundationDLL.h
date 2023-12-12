#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Types/Uuid.h>
#include <QColor>
#include <QMetaType>
#include <ToolsFoundation/ToolsFoundationDLL.h>

// Configure the DLL Import/Export Define
#if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GUIFOUNDATION_LIB
#    define PLASMA_GUIFOUNDATION_DLL PLASMA_DECL_EXPORT
#  else
#    define PLASMA_GUIFOUNDATION_DLL PLASMA_DECL_IMPORT
#  endif
#else
#  define PLASMA_GUIFOUNDATION_DLL
#endif

class QWidget;
class QObject;


Q_DECLARE_METATYPE(plUuid);

/// \brief Calls setUpdatesEnabled(false) on all given QObjects, and the reverse in the destructor. Can be nested.
class PLASMA_GUIFOUNDATION_DLL plQtScopedUpdatesDisabled
{
public:
  plQtScopedUpdatesDisabled(QWidget* pWidget1, QWidget* pWidget2 = nullptr, QWidget* pWidget3 = nullptr, QWidget* pWidget4 = nullptr,
    QWidget* pWidget5 = nullptr, QWidget* pWidget6 = nullptr);
  ~plQtScopedUpdatesDisabled();

private:
  QWidget* m_pWidgets[6];
};


/// \brief Calls blockSignals(true) on all given QObjects, and the reverse in the destructor. Can be nested.
class PLASMA_GUIFOUNDATION_DLL plQtScopedBlockSignals
{
public:
  plQtScopedBlockSignals(QObject* pObject1, QObject* pObject2 = nullptr, QObject* pObject3 = nullptr, QObject* pObject4 = nullptr,
    QObject* pObject5 = nullptr, QObject* pObject6 = nullptr);
  ~plQtScopedBlockSignals();

private:
  QObject* m_pObjects[6];
};

PLASMA_ALWAYS_INLINE QColor plToQtColor(const plColorGammaUB& c)
{
  return QColor(c.r, c.g, c.b, c.a);
}

PLASMA_ALWAYS_INLINE plColorGammaUB qtToPlColor(const QColor& c)
{
  return plColorGammaUB(c.red(), c.green(), c.blue(), c.alpha());
}
