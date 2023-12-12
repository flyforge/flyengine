#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/DeduplicationReadContext.h>
#include <Foundation/IO/DeduplicationWriteContext.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  struct RefCountedVec3 : public plRefCounted
  {
    RefCountedVec3() = default;
    RefCountedVec3(const plVec3& v)
      : m_v(v)
    {
    }

    plResult Serialize(plStreamWriter& inout_stream) const
    {
      inout_stream << m_v;
      return PLASMA_SUCCESS;
    }

    plResult Deserialize(plStreamReader& inout_stream)
    {
      inout_stream >> m_v;
      return PLASMA_SUCCESS;
    }

    plVec3 m_v;
  };

  struct ComplexComponent
  {
    plTransform* m_pTransform = nullptr;
    plVec3* m_pPosition = nullptr;
    plSharedPtr<RefCountedVec3> m_pScale;
    plUInt32 m_uiIndex = plInvalidIndex;

    plResult Serialize(plStreamWriter& inout_stream) const
    {
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationWriteContext::GetContext()->WriteObject(inout_stream, m_pTransform));
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationWriteContext::GetContext()->WriteObject(inout_stream, m_pPosition));
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationWriteContext::GetContext()->WriteObject(inout_stream, m_pScale));

      inout_stream << m_uiIndex;
      return PLASMA_SUCCESS;
    }

    plResult Deserialize(plStreamReader& inout_stream)
    {
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationReadContext::GetContext()->ReadObject(inout_stream, m_pTransform));
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationReadContext::GetContext()->ReadObject(inout_stream, m_pPosition));
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationReadContext::GetContext()->ReadObject(inout_stream, m_pScale));

      inout_stream >> m_uiIndex;
      return PLASMA_SUCCESS;
    }
  };

  struct ComplexObject
  {
    plDynamicArray<plUniquePtr<plTransform>> m_Transforms;
    plDynamicArray<plVec3> m_Positions;
    plDynamicArray<plSharedPtr<RefCountedVec3>> m_Scales;

    plDynamicArray<ComplexComponent> m_Components;

    plMap<plUInt32, plTransform*> m_TransformMap;
    plSet<plVec3*> m_UniquePositions;

    plResult Serialize(plStreamWriter& inout_stream) const
    {
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationWriteContext::GetContext()->WriteArray(inout_stream, m_Transforms));
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationWriteContext::GetContext()->WriteArray(inout_stream, m_Positions));
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationWriteContext::GetContext()->WriteArray(inout_stream, m_Scales));
      PLASMA_SUCCEED_OR_RETURN(
        plDeduplicationWriteContext::GetContext()->WriteMap(inout_stream, m_TransformMap, plDeduplicationWriteContext::WriteMapMode::DedupValue));
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationWriteContext::GetContext()->WriteSet(inout_stream, m_UniquePositions));
      PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Components));
      return PLASMA_SUCCESS;
    }

    plResult Deserialize(plStreamReader& inout_stream)
    {
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationReadContext::GetContext()->ReadArray(inout_stream, m_Transforms));
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationReadContext::GetContext()->ReadArray(inout_stream, m_Positions,
        nullptr)); // should not allocate anything
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationReadContext::GetContext()->ReadArray(inout_stream, m_Scales));
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationReadContext::GetContext()->ReadMap(
        inout_stream, m_TransformMap, plDeduplicationReadContext::ReadMapMode::DedupValue, nullptr, nullptr));           // should not allocate anything
      PLASMA_SUCCEED_OR_RETURN(plDeduplicationReadContext::GetContext()->ReadSet(inout_stream, m_UniquePositions, nullptr)); // should not allocate anything
      PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Components));
      return PLASMA_SUCCESS;
    }
  };
} // namespace

PLASMA_CREATE_SIMPLE_TEST(IO, DeduplicationContext)
{
  plDefaultMemoryStreamStorage streamStorage;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Writer")
  {
    plMemoryStreamWriter writer(&streamStorage);

    plDeduplicationWriteContext dedupWriteContext;

    ComplexObject obj;
    for (plUInt32 i = 0; i < 20; ++i)
    {
      obj.m_Transforms.ExpandAndGetRef() = PLASMA_DEFAULT_NEW(plTransform, plVec3(static_cast<float>(i), 0, 0));
      obj.m_Positions.ExpandAndGetRef() = plVec3(1, 2, static_cast<float>(i));
      obj.m_Scales.ExpandAndGetRef() = PLASMA_DEFAULT_NEW(RefCountedVec3, plVec3(0, static_cast<float>(i), 0));
    }

    for (plUInt32 i = 0; i < 10; ++i)
    {
      auto& component = obj.m_Components.ExpandAndGetRef();
      component.m_uiIndex = i * 2;
      component.m_pTransform = obj.m_Transforms[component.m_uiIndex].Borrow();
      component.m_pPosition = &obj.m_Positions[component.m_uiIndex];
      component.m_pScale = obj.m_Scales[component.m_uiIndex];

      obj.m_TransformMap.Insert(i, obj.m_Transforms[i].Borrow());
      obj.m_UniquePositions.Insert(&obj.m_Positions[i]);
    }



    PLASMA_TEST_BOOL(obj.Serialize(writer).Succeeded());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Reader")
  {
    plMemoryStreamReader reader(&streamStorage);

    plDeduplicationReadContext dedupReadContext;

    ComplexObject obj;
    PLASMA_TEST_BOOL(obj.Deserialize(reader).Succeeded());

    PLASMA_TEST_INT(obj.m_Transforms.GetCount(), 20);
    PLASMA_TEST_INT(obj.m_Positions.GetCount(), 20);
    PLASMA_TEST_INT(obj.m_Scales.GetCount(), 20);
    PLASMA_TEST_INT(obj.m_TransformMap.GetCount(), 10);
    PLASMA_TEST_INT(obj.m_UniquePositions.GetCount(), 10);
    PLASMA_TEST_INT(obj.m_Components.GetCount(), 10);

    for (plUInt32 i = 0; i < obj.m_Components.GetCount(); ++i)
    {
      auto& component = obj.m_Components[i];

      PLASMA_TEST_BOOL(component.m_pTransform == obj.m_Transforms[component.m_uiIndex].Borrow());
      PLASMA_TEST_BOOL(component.m_pPosition == &obj.m_Positions[component.m_uiIndex]);
      PLASMA_TEST_BOOL(component.m_pScale == obj.m_Scales[component.m_uiIndex]);

      PLASMA_TEST_BOOL(component.m_pTransform->m_vPosition == plVec3(static_cast<float>(i) * 2, 0, 0));
      PLASMA_TEST_BOOL(*component.m_pPosition == plVec3(1, 2, static_cast<float>(i) * 2));
      PLASMA_TEST_BOOL(component.m_pScale->m_v == plVec3(0, static_cast<float>(i) * 2, 0));
    }

    for (plUInt32 i = 0; i < 10; ++i)
    {
      if (PLASMA_TEST_BOOL(obj.m_TransformMap.GetValue(i) != nullptr))
      {
        PLASMA_TEST_BOOL(*obj.m_TransformMap.GetValue(i) == obj.m_Transforms[i].Borrow());
      }

      PLASMA_TEST_BOOL(obj.m_UniquePositions.Contains(&obj.m_Positions[i]));
    }
  }
}
