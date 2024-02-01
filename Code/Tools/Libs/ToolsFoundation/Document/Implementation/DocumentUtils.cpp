#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentUtils.h>

plStatus plDocumentUtils::IsValidSaveLocationForDocument(plStringView sDocument, const plDocumentTypeDescriptor** out_pTypeDesc)
{
  const plDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (plDocumentManager::FindDocumentTypeFromPath(sDocument, true, pTypeDesc).Failed())
  {
    plStringBuilder sTemp;
    sTemp.SetFormat("The selected file extension '{0}' is not registered with any known type.\nCannot create file '{1}'",
      plPathUtils::GetFileExtension(sDocument), sDocument);
    return plStatus(sTemp.GetData());
  }

  if (plDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(sDocument))
  {
    return plStatus("The selected document is already open. You need to close the document before you can re-create it.");
  }

  if (out_pTypeDesc)
  {
    *out_pTypeDesc = pTypeDesc;
  }
  return plStatus(PL_SUCCESS);
}
