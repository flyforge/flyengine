#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Uuid.h>
#include <QColor>
#include <QDataStream>
#include <QMetaType>
#include <ToolsFoundation/ToolsFoundationDLL.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GUIFOUNDATION_LIB
#    define PL_GUIFOUNDATION_DLL PL_DECL_EXPORT
#  else
#    define PL_GUIFOUNDATION_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_GUIFOUNDATION_DLL
#endif

class QWidget;
class QObject;


Q_DECLARE_METATYPE(plUuid);

/// \brief Calls setUpdatesEnabled(false) on all given QObjects, and the reverse in the destructor. Can be nested.
class PL_GUIFOUNDATION_DLL plQtScopedUpdatesDisabled
{
public:
  plQtScopedUpdatesDisabled(QWidget* pWidget1, QWidget* pWidget2 = nullptr, QWidget* pWidget3 = nullptr, QWidget* pWidget4 = nullptr,
    QWidget* pWidget5 = nullptr, QWidget* pWidget6 = nullptr);
  ~plQtScopedUpdatesDisabled();

private:
  QWidget* m_pWidgets[6];
};


/// \brief Calls blockSignals(true) on all given QObjects, and the reverse in the destructor. Can be nested.
class PL_GUIFOUNDATION_DLL plQtScopedBlockSignals
{
public:
  plQtScopedBlockSignals(QObject* pObject1, QObject* pObject2 = nullptr, QObject* pObject3 = nullptr, QObject* pObject4 = nullptr,
    QObject* pObject5 = nullptr, QObject* pObject6 = nullptr);
  ~plQtScopedBlockSignals();

private:
  QObject* m_pObjects[6];
};

PL_ALWAYS_INLINE QColor plToQtColor(const plColorGammaUB& c)
{
  return QColor(c.r, c.g, c.b, c.a);
}

PL_ALWAYS_INLINE plColorGammaUB qtToPlColor(const QColor& c)
{
  return plColorGammaUB(c.red(), c.green(), c.blue(), c.alpha());
}

PL_ALWAYS_INLINE plString qtToPlString(const QString& sString)
{
  QByteArray data = sString.toUtf8();
  return plString(plStringView(data.data(), static_cast<plUInt32>(data.size())));
}

PL_ALWAYS_INLINE QString plMakeQString(plStringView sString)
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

template <typename T>
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
