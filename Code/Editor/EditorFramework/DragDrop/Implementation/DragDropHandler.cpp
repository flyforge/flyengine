#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropHandler.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDragDropHandler, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plDragDropHandler* plDragDropHandler::s_pActiveDnD = nullptr;

plDragDropHandler::plDragDropHandler() = default;


plDragDropHandler* plDragDropHandler::FindDragDropHandler(const plDragDropInfo* pInfo)
{
  float fBestValue = 0.0f;
  plDragDropHandler* pBestDnD = nullptr;

  plRTTI::ForEachDerivedType<plDragDropHandler>(
    [&](const plRTTI* pRtti) {
      plDragDropHandler* pDnD = pRtti->GetAllocator()->Allocate<plDragDropHandler>();

      const float fValue = pDnD->CanHandle(pInfo);
      if (fValue > fBestValue)
      {
        if (pBestDnD != nullptr)
        {
          pBestDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(pBestDnD);
        }

        fBestValue = fValue;
        pBestDnD = pDnD;
      }
      else
      {
        pDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(pDnD);
      }
    },
    plRTTI::ForEachOptions::ExcludeNonAllocatable);

  return pBestDnD;
}

bool plDragDropHandler::BeginDragDropOperation(const plDragDropInfo* pInfo, plDragDropConfig* pConfigToFillOut)
{
  PL_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  plDragDropHandler* pHandler = FindDragDropHandler(pInfo);

  if (pHandler != nullptr)
  {
    if (pConfigToFillOut != nullptr)
      pHandler->RequestConfiguration(pConfigToFillOut);

    s_pActiveDnD = pHandler;
    s_pActiveDnD->OnDragBegin(pInfo);
    return true;
  }

  return false;
}

void plDragDropHandler::UpdateDragDropOperation(const plDragDropInfo* pInfo)
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDragUpdate(pInfo);
}

void plDragDropHandler::FinishDragDrop(const plDragDropInfo* pInfo)
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDrop(pInfo);

  s_pActiveDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(s_pActiveDnD);
  s_pActiveDnD = nullptr;
}

void plDragDropHandler::CancelDragDrop()
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDragCancel();

  s_pActiveDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(s_pActiveDnD);
  s_pActiveDnD = nullptr;
}

bool plDragDropHandler::CanDropOnly(const plDragDropInfo* pInfo)
{
  PL_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  plDragDropHandler* pHandler = FindDragDropHandler(pInfo);

  if (pHandler != nullptr)
  {
    pHandler->GetDynamicRTTI()->GetAllocator()->Deallocate(pHandler);
    return true;
  }

  return false;
}

bool plDragDropHandler::DropOnly(const plDragDropInfo* pInfo)
{
  PL_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  if (BeginDragDropOperation(pInfo))
  {
    FinishDragDrop(pInfo);
    return true;
  }

  return false;
}
