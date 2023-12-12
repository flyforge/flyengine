#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>

plDocumentObjectVisitor::plDocumentObjectVisitor(
  const plDocumentObjectManager* pManager, const char* szChildrenProperty /*= "Children"*/, const char* szRootProperty /*= "Children"*/)
  : m_pManager(pManager)
  , m_sChildrenProperty(szChildrenProperty)
  , m_sRootProperty(szRootProperty)
{
  const plAbstractProperty* pRootProp = m_pManager->GetRootObject()->GetType()->FindPropertyByName(szRootProperty);
  PLASMA_ASSERT_DEV(pRootProp, "Given root property '{0}' does not exist on root object", szRootProperty);
  PLASMA_ASSERT_DEV(pRootProp->GetCategory() == plPropertyCategory::Set || pRootProp->GetCategory() == plPropertyCategory::Array,
    "Traverser only works on arrays and sets.");

  // const plAbstractProperty* pChildProp = pRootProp->GetSpecificType()->FindPropertyByName(szChildrenProperty);
  // PLASMA_ASSERT_DEV(pChildProp, "Given child property '{0}' does not exist", szChildrenProperty);
  // PLASMA_ASSERT_DEV(pChildProp->GetCategory() == plPropertyCategory::Set || pRootProp->GetCategory() == plPropertyCategory::Array, "Traverser
  // only works on arrays and sets.");
}

void plDocumentObjectVisitor::Visit(const plDocumentObject* pObject, bool bVisitStart, VisitorFunction function)
{
  const char* szProperty = m_sChildrenProperty;
  if (pObject == nullptr || pObject == m_pManager->GetRootObject())
  {
    pObject = m_pManager->GetRootObject();
    szProperty = m_sRootProperty;
  }

  if (!bVisitStart || function(pObject))
  {
    TraverseChildren(pObject, szProperty, function);
  }
}

void plDocumentObjectVisitor::TraverseChildren(const plDocumentObject* pObject, const char* szProperty, VisitorFunction& function)
{
  const plInt32 iChildren = pObject->GetTypeAccessor().GetCount(szProperty);
  for (plInt32 i = 0; i < iChildren; i++)
  {
    plVariant obj = pObject->GetTypeAccessor().GetValue(szProperty, i);
    PLASMA_ASSERT_DEBUG(obj.IsValid() && obj.IsA<plUuid>(), "null obj found during traversal.");
    const plDocumentObject* pChild = m_pManager->GetObject(obj.Get<plUuid>());
    if (function(pChild))
    {
      TraverseChildren(pChild, m_sChildrenProperty, function);
    }
  }
}
