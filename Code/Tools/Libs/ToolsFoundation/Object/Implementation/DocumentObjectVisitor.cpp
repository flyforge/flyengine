#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>

plDocumentObjectVisitor::plDocumentObjectVisitor(
  const plDocumentObjectManager* pManager, plStringView sChildrenProperty /*= "Children"*/, plStringView sRootProperty /*= "Children"*/)
  : m_pManager(pManager)
  , m_sChildrenProperty(sChildrenProperty)
  , m_sRootProperty(sRootProperty)
{
  const plAbstractProperty* pRootProp = m_pManager->GetRootObject()->GetType()->FindPropertyByName(sRootProperty);
  PLASMA_ASSERT_DEV(pRootProp, "Given root property '{0}' does not exist on root object", sRootProperty);
  PLASMA_ASSERT_DEV(pRootProp->GetCategory() == plPropertyCategory::Set || pRootProp->GetCategory() == plPropertyCategory::Array,
    "Traverser only works on arrays and sets.");

  // const plAbstractProperty* pChildProp = pRootProp->GetSpecificType()->FindPropertyByName(szChildrenProperty);
  // PLASMA_ASSERT_DEV(pChildProp, "Given child property '{0}' does not exist", szChildrenProperty);
  // PLASMA_ASSERT_DEV(pChildProp->GetCategory() == plPropertyCategory::Set || pRootProp->GetCategory() == plPropertyCategory::Array, "Traverser
  // only works on arrays and sets.");
}

void plDocumentObjectVisitor::Visit(const plDocumentObject* pObject, bool bVisitStart, VisitorFunction function)
{
  plStringView sProperty = m_sChildrenProperty;
  if (pObject == nullptr || pObject == m_pManager->GetRootObject())
  {
    pObject = m_pManager->GetRootObject();
    sProperty = m_sRootProperty;
  }

  if (!bVisitStart || function(pObject))
  {
    TraverseChildren(pObject, sProperty, function);
  }
}

void plDocumentObjectVisitor::TraverseChildren(const plDocumentObject* pObject, plStringView sProperty, VisitorFunction& function)
{
  const plInt32 iChildren = pObject->GetTypeAccessor().GetCount(sProperty);
  for (plInt32 i = 0; i < iChildren; i++)
  {
    plVariant obj = pObject->GetTypeAccessor().GetValue(sProperty, i);
    PLASMA_ASSERT_DEBUG(obj.IsValid() && obj.IsA<plUuid>(), "null obj found during traversal.");
    const plDocumentObject* pChild = m_pManager->GetObject(obj.Get<plUuid>());
    if (function(pChild))
    {
      TraverseChildren(pChild, m_sChildrenProperty, function);
    }
  }
}
