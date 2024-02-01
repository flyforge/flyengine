#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Debugging/LineToComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plLineToComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    // BEGIN-DOCS-CODE-SNIPPET: object-reference-property
    PL_ACCESSOR_PROPERTY("Target", GetLineToTargetGuid, SetLineToTargetGuid)->AddAttributes(new plGameObjectReferenceAttribute()),
    // END-DOCS-CODE-SNIPPET
    PL_MEMBER_PROPERTY("Color", m_LineColor)->AddAttributes(new plDefaultValueAttribute(plColor::Orange)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Utilities/Debug"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLineToComponent::plLineToComponent() = default;
plLineToComponent::~plLineToComponent() = default;

void plLineToComponent::Update()
{
  if (m_hTargetObject.IsInvalidated())
    return;

  plGameObject* pTarget = nullptr;
  if (!GetWorld()->TryGetObject(m_hTargetObject, pTarget))
  {
    m_hTargetObject.Invalidate();
    return;
  }

  plDynamicArray<plDebugRenderer::Line> lines;

  auto& line = lines.ExpandAndGetRef();
  line.m_start = GetOwner()->GetGlobalPosition();
  line.m_end = pTarget->GetGlobalPosition();

  plDebugRenderer::DrawLines(GetWorld(), lines, m_LineColor);
}

void plLineToComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hTargetObject);
  s << m_LineColor;
}

void plLineToComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  m_hTargetObject = inout_stream.ReadGameObjectHandle();
  s >> m_LineColor;
}

void plLineToComponent::SetLineToTarget(const plGameObjectHandle& hTargetObject)
{
  m_hTargetObject = hTargetObject;
}

// BEGIN-DOCS-CODE-SNIPPET: object-reference-funcs
void plLineToComponent::SetLineToTargetGuid(const char* szTargetGuid)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (resolver.IsValid())
  {
    // tell the resolver our component handle and the name of the property for the object reference
    m_hTargetObject = resolver(szTargetGuid, GetHandle(), "Target");
  }
}

const char* plLineToComponent::GetLineToTargetGuid() const
{
  // this function is never called
  return nullptr;
}
// END-DOCS-CODE-SNIPPET

//////////////////////////////////////////////////////////////////////////

plLineToComponentManager::plLineToComponentManager(plWorld* pWorld)
  : SUPER(pWorld)
{
}

void plLineToComponentManager::Initialize()
{
  auto desc = plWorldModule::UpdateFunctionDesc(plWorldModule::UpdateFunction(&plLineToComponentManager::Update, this), "plLineToComponentManager::Update");
  desc.m_bOnlyUpdateWhenSimulating = false;
  desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostTransform;

  this->RegisterUpdateFunction(desc);
}


void plLineToComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

