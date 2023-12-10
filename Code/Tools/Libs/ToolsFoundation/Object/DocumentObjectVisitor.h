#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Delegate.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plDocumentObjectManager;
class plDocumentObject;

/// \brief Implements visitor pattern for content of the document object manager.
class PLASMA_TOOLSFOUNDATION_DLL plDocumentObjectVisitor
{
public:
  /// \brief Constructor
  ///
  /// \param pManager
  ///   Manager that will be iterated through.
  /// \param szChildrenProperty
  ///   Name of the property that is used for finding children on an object.
  /// \param szRootProperty
  ///   Same as szChildrenProperty, but for the root object of the document.
  plDocumentObjectVisitor(
    const plDocumentObjectManager* pManager, plStringView sChildrenProperty = "Children", plStringView sRootProperty = "Children");

  using VisitorFunction = plDelegate<bool(const plDocumentObject*)>;
  /// \brief Executes depth first traversal starting at the given node.
  ///
  /// \param pObject
  ///   Object to start traversal at.
  /// \param bVisitStart
  ///   If true, function will be executed for the start object as well.
  /// \param function
  ///   Functions executed for each visited object. Should true if the object's children should be traversed.
  void Visit(const plDocumentObject* pObject, bool bVisitStart, VisitorFunction function);

private:
  void TraverseChildren(const plDocumentObject* pObject, plStringView sProperty, VisitorFunction& function);

  const plDocumentObjectManager* m_pManager = nullptr;
  plString m_sChildrenProperty;
  plString m_sRootProperty;
};
