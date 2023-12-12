#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Versioning);

struct plPatchTestBase
{
public:
  plPatchTestBase()
  {
    m_string = "Base";
    m_string2 = "";
  }

  plString m_string;
  plString m_string2;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plPatchTestBase);

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plPatchTestBase, plNoBase, 1, plRTTIDefaultAllocator<plPatchTestBase>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("String", m_string),
    PLASMA_MEMBER_PROPERTY("String2", m_string2),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

struct plPatchTest : public plPatchTestBase
{
public:
  plPatchTest() { m_iInt32 = 1; }

  plInt32 m_iInt32;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plPatchTest);

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plPatchTest, plPatchTestBase, 1, plRTTIDefaultAllocator<plPatchTest>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Int", m_iInt32),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Patch class
  class plPatchTestP : public plGraphPatch
  {
  public:
    plPatchTestP()
      : plGraphPatch("plPatchTestP", 2)
    {
    }
    virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
    {
      pNode->RenameProperty("Int", "IntRenamed");
      pNode->ChangeProperty("IntRenamed", 2);
    }
  };
  plPatchTestP g_plPatchTestP;

  /// Patch base class
  class plPatchTestBaseBP : public plGraphPatch
  {
  public:
    plPatchTestBaseBP()
      : plGraphPatch("plPatchTestBaseBP", 2)
    {
    }
    virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
    {
      pNode->ChangeProperty("String", "BaseClassPatched");
    }
  };
  plPatchTestBaseBP g_plPatchTestBaseBP;

  /// Rename class
  class plPatchTestRN : public plGraphPatch
  {
  public:
    plPatchTestRN()
      : plGraphPatch("plPatchTestRN", 2)
    {
    }
    virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
    {
      ref_context.RenameClass("plPatchTestRN2");
      pNode->ChangeProperty("String", "RenameExecuted");
    }
  };
  plPatchTestRN g_plPatchTestRN;

  /// Patch renamed class to v3
  class plPatchTestRN2 : public plGraphPatch
  {
  public:
    plPatchTestRN2()
      : plGraphPatch("plPatchTestRN2", 3)
    {
    }
    virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
    {
      pNode->ChangeProperty("String2", "Patched");
    }
  };
  plPatchTestRN2 g_plPatchTestRN2;

  /// Change base class
  class plPatchTestCB : public plGraphPatch
  {
  public:
    plPatchTestCB()
      : plGraphPatch("plPatchTestCB", 2)
    {
    }
    virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
    {
      plVersionKey bases[] = {{"plPatchTestBaseBP", 1}};
      ref_context.ChangeBaseClass(bases);
      pNode->ChangeProperty("String2", "ChangedBase");
    }
  };
  plPatchTestCB g_plPatchTestCB;

  void ReplaceTypeName(plAbstractObjectGraph& ref_graph, plAbstractObjectGraph& ref_typesGraph, const char* szOldName, const char* szNewName)
  {
    for (auto it : ref_graph.GetAllNodes())
    {
      auto* pNode = it.Value();

      if (szOldName == pNode->GetType())
        pNode->SetType(szNewName);
    }

    for (auto it : ref_typesGraph.GetAllNodes())
    {
      auto* pNode = it.Value();

      if ("plReflectedTypeDescriptor" == pNode->GetType())
      {
        if (auto* pProp = pNode->FindProperty("TypeName"))
        {
          if (plStringUtils::IsEqual(szOldName, pProp->m_Value.Get<plString>()))
            pProp->m_Value = szNewName;
        }
        if (auto* pProp = pNode->FindProperty("ParentTypeName"))
        {
          if (plStringUtils::IsEqual(szOldName, pProp->m_Value.Get<plString>()))
            pProp->m_Value = szNewName;
        }
      }
    }
  }

  plAbstractObjectNode* SerializeObject(plAbstractObjectGraph& ref_graph, plAbstractObjectGraph& ref_typesGraph, const plRTTI* pRtti, void* pObject)
  {
    plAbstractObjectNode* pNode = nullptr;
    {
      // Object
      plRttiConverterContext context;
      plRttiConverterWriter rttiConverter(&ref_graph, &context, true, true);
      context.RegisterObject(plUuid::StableUuidForString(pRtti->GetTypeName()), pRtti, pObject);
      pNode = rttiConverter.AddObjectToGraph(pRtti, pObject, "ROOT");
    }
    {
      // Types
      plSet<const plRTTI*> types;
      types.Insert(pRtti);
      plReflectionUtils::GatherDependentTypes(pRtti, types);
      plToolsSerializationUtils::SerializeTypes(types, ref_typesGraph);
    }
    return pNode;
  }

  void PatchGraph(plAbstractObjectGraph& ref_graph, plAbstractObjectGraph& ref_typesGraph)
  {
    plGraphVersioning::GetSingleton()->PatchGraph(&ref_typesGraph);
    plGraphVersioning::GetSingleton()->PatchGraph(&ref_graph, &ref_typesGraph);
  }
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Versioning, GraphPatch)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PatchClass")
  {
    plAbstractObjectGraph graph;
    plAbstractObjectGraph typesGraph;

    plPatchTest data;
    data.m_iInt32 = 5;
    plAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, plGetStaticRTTI<plPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "plPatchTest", "plPatchTestP");
    PatchGraph(graph, typesGraph);

    plAbstractObjectNode::Property* pInt = pNode->FindProperty("IntRenamed");
    PLASMA_TEST_INT(2, pInt->m_Value.Get<plInt32>());
    PLASMA_TEST_BOOL(pNode->FindProperty("Int") == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PatchBaseClass")
  {
    plAbstractObjectGraph graph;
    plAbstractObjectGraph typesGraph;

    plPatchTest data;
    data.m_string = "Unpatched";
    plAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, plGetStaticRTTI<plPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "plPatchTestBase", "plPatchTestBaseBP");
    PatchGraph(graph, typesGraph);

    plAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    PLASMA_TEST_STRING(pString->m_Value.Get<plString>(), "BaseClassPatched");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RenameClass")
  {
    plAbstractObjectGraph graph;
    plAbstractObjectGraph typesGraph;

    plPatchTest data;
    data.m_string = "NotRenamed";
    plAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, plGetStaticRTTI<plPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "plPatchTest", "plPatchTestRN");
    PatchGraph(graph, typesGraph);

    plAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    PLASMA_TEST_BOOL(pString->m_Value.Get<plString>() == "RenameExecuted");
    PLASMA_TEST_STRING(pNode->GetType(), "plPatchTestRN2");
    PLASMA_TEST_INT(pNode->GetTypeVersion(), 3);
    plAbstractObjectNode::Property* pString2 = pNode->FindProperty("String2");
    PLASMA_TEST_BOOL(pString2->m_Value.Get<plString>() == "Patched");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ChangeBaseClass")
  {
    plAbstractObjectGraph graph;
    plAbstractObjectGraph typesGraph;

    plPatchTest data;
    data.m_string = "NotPatched";
    plAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, plGetStaticRTTI<plPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "plPatchTest", "plPatchTestCB");
    PatchGraph(graph, typesGraph);

    plAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    PLASMA_TEST_STRING(pString->m_Value.Get<plString>(), "BaseClassPatched");
    PLASMA_TEST_INT(pNode->GetTypeVersion(), 2);
    plAbstractObjectNode::Property* pString2 = pNode->FindProperty("String2");
    PLASMA_TEST_STRING(pString2->m_Value.Get<plString>(), "ChangedBase");
  }
}
