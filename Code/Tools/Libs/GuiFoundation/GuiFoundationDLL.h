#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Types/Uuid.h>
#include <QColor>
#include <QMetaType>
#include <ToolsFoundation/ToolsFoundationDLL.h>
#include <Foundation/Strings/String.h>
#include <QDataStream>

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

PLASMA_ALWAYS_INLINE plColorGammaUB qtToPlasmaColor(const QColor& c)
{
  return plColorGammaUB(c.red(), c.green(), c.blue(), c.alpha());
}

PLASMA_ALWAYS_INLINE plString qtToPlasmaString(const QString& sString)
{
  QByteArray data = sString.toUtf8();
  return plString(plStringView(data.data(), data.size()));
}

PLASMA_ALWAYS_INLINE QString plMakeQString(plStringView sString)
{
  return QString::fromUtf8(sString.GetStartPointer(), sString.GetElementCount());
}

template <typename T>
void operator>>(QDataStream& inout_stream, T*& rhs)
{
  void* p = nullptr;
  uint len = sizeof(void*);
  inout_stream.readRawData((char*)&p, len);
  rhs = (T*)p;
}


template <typename T>
void operator<<(QDataStream& inout_stream, T* rhs)
{
  inout_stream.writeRawData((const char*)&rhs, sizeof(void*));
}

template<typename T>
void operator>>(QDataStream& inout_stream, plDynamicArray<T>& rhs)
{
  plUInt32 uiIndices = 0;
  inout_stream >> uiIndices;
  rhs.Clear();
  rhs.Reserve(uiIndices);

  for (int i = 0; i < uiIndices; ++i)
  {
    T obj = {};
    inout_stream >> obj;
    rhs.PushBack(obj);
  }
}

template <typename T>
void operator<<(QDataStream& inout_stream, plDynamicArray<T>& rhs)
{
  plUInt32 iIndices = rhs.GetCount();
  inout_stream << iIndices;

  for (plUInt32 i = 0; i < iIndices; ++i)
  {
    inout_stream << rhs[i];
  }
}
