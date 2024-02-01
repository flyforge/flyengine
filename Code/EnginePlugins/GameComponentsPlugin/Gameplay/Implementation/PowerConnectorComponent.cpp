#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Gameplay/PowerConnectorComponent.h>

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plEventMsgSetPowerInput);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEventMsgSetPowerInput, 1, plRTTIDefaultAllocator<plEventMsgSetPowerInput>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("PrevValue", m_uiPrevValue),
    PL_MEMBER_PROPERTY("NewValue", m_uiNewValue),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_COMPONENT_TYPE(plPowerConnectorComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Output", GetOutput, SetOutput),
    PL_ACCESSOR_PROPERTY("Buddy", DummyGetter, SetBuddyReference)->AddAttributes(new plGameObjectReferenceAttribute()),
    PL_ACCESSOR_PROPERTY("ConnectedTo", DummyGetter, SetConnectedToReference)->AddAttributes(new plGameObjectReferenceAttribute()),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgSensorDetectedObjectsChanged, OnMsgSensorDetectedObjectsChanged),
    PL_MESSAGE_HANDLER(plMsgObjectGrabbed, OnMsgObjectGrabbed),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(IsConnected),
    PL_SCRIPT_FUNCTION_PROPERTY(IsAttached),
    PL_SCRIPT_FUNCTION_PROPERTY(Detach),
    // PL_SCRIPT_FUNCTION_PROPERTY(Attach, In, "Object"), // not supported (yet)
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE;
// clang-format on

void plPowerConnectorComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hBuddy);
  inout_stream.WriteGameObjectHandle(m_hConnectedTo);

  s << m_uiOutput;
}

void plPowerConnectorComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  m_hBuddy = inout_stream.ReadGameObjectHandle();
  m_hConnectedTo = inout_stream.ReadGameObjectHandle();

  s >> m_uiOutput;
}

void plPowerConnectorComponent::ConnectToSocket(plGameObjectHandle hSocket)
{
  if (IsConnected())
    return;

  if (GetOwner()->GetWorld()->GetClock().GetAccumulatedTime() - m_DetachTime < plTime::MakeFromSeconds(1))
  {
    // recently detached -> wait a bit before allowing to attach again
    return;
  }

  Attach(hSocket);
}

void plPowerConnectorComponent::SetOutput(plUInt16 value)
{
  if (m_uiOutput == value)
    return;

  m_uiOutput = value;

  OutputChanged(m_uiOutput);
}

void plPowerConnectorComponent::SetInput(plUInt16 value)
{
  if (m_uiInput == value)
    return;

  InputChanged(m_uiInput, value);
  m_uiInput = value;
}

void plPowerConnectorComponent::SetBuddyReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetBuddy(resolver(szReference, GetHandle(), "Buddy"));
}

void plPowerConnectorComponent::SetBuddy(plGameObjectHandle hNewBuddy)
{
  if (m_hBuddy == hNewBuddy)
    return;

  if (!IsActiveAndInitialized())
  {
    m_hBuddy = hNewBuddy;
    return;
  }

  plGameObjectHandle hPrevBuddy = m_hBuddy;
  m_hBuddy = {};

  plGameObject* pBuddy;
  if (GetOwner()->GetWorld()->TryGetObject(hPrevBuddy, pBuddy))
  {
    plPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetOutput(0);
      pConnector->SetBuddy({});
    }
  }

  m_hBuddy = hNewBuddy;

  if (GetOwner()->GetWorld()->TryGetObject(hNewBuddy, pBuddy))
  {
    plPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetBuddy(GetOwner()->GetHandle());
      pConnector->SetOutput(m_uiInput);
    }
  }
}

void plPowerConnectorComponent::SetConnectedToReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetConnectedTo(resolver(szReference, GetHandle(), "ConnectedTo"));
}

void plPowerConnectorComponent::SetConnectedTo(plGameObjectHandle hNewConnectedTo)
{
  if (m_hConnectedTo == hNewConnectedTo)
    return;

  if (!IsActiveAndInitialized())
  {
    m_hConnectedTo = hNewConnectedTo;
    return;
  }

  plGameObjectHandle hPrevConnectedTo = m_hConnectedTo;
  m_hConnectedTo = {};

  plGameObject* pConnectedTo;
  if (GetOwner()->GetWorld()->TryGetObject(hPrevConnectedTo, pConnectedTo))
  {
    plPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetInput(0);
      pConnector->SetConnectedTo({});
    }
  }

  m_hConnectedTo = hNewConnectedTo;

  if (GetOwner()->GetWorld()->TryGetObject(hNewConnectedTo, pConnectedTo))
  {
    plPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetConnectedTo(GetOwner()->GetHandle());
      pConnector->SetInput(m_uiOutput);
    }
  }

  if (hNewConnectedTo.IsInvalidated() && IsAttached())
  {
    // make sure that if we get disconnected, we also clean up our detachment state
    Detach();
  }
}

bool plPowerConnectorComponent::IsConnected() const
{
  // since connectors automatically disconnect themselves from their peers upon destruction, this should be sufficient (no need to check object for existence)
  return !m_hConnectedTo.IsInvalidated();
}

bool plPowerConnectorComponent::IsAttached() const
{
  return !m_hAttachPoint.IsInvalidated();
}

void plPowerConnectorComponent::OnDeactivated()
{
  Detach();
  SetBuddy({});

  SUPER::OnDeactivated();
}

void plPowerConnectorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  plGameObjectHandle hAlreadyConnectedTo = m_hConnectedTo;
  m_hConnectedTo.Invalidate();

  if (!hAlreadyConnectedTo.IsInvalidated())
  {
    Attach(hAlreadyConnectedTo);
  }

  if (m_uiInput != 0)
  {
    InputChanged(0, m_uiInput);
  }

  if (m_uiOutput != 0)
  {
    OutputChanged(m_uiOutput);
  }
}

void plPowerConnectorComponent::OnMsgSensorDetectedObjectsChanged(plMsgSensorDetectedObjectsChanged& msg)
{
  if (!msg.m_DetectedObjects.IsEmpty())
  {
    ConnectToSocket(msg.m_DetectedObjects[0]);
  }
}

void plPowerConnectorComponent::OnMsgObjectGrabbed(plMsgObjectGrabbed& msg)
{
  if (msg.m_bGotGrabbed)
  {
    Detach();

    m_hGrabbedBy = msg.m_hGrabbedBy;

    if (plGameObject* pSensor = GetOwner()->FindChildByName("ActiveWhenGrabbed"))
    {
      pSensor->SetActiveFlag(true);
    }
  }
  else
  {
    m_hGrabbedBy.Invalidate();

    if (plGameObject* pSensor = GetOwner()->FindChildByName("ActiveWhenGrabbed"))
    {
      pSensor->SetActiveFlag(false);
    }
  }
}

void plPowerConnectorComponent::Attach(plGameObjectHandle hSocket)
{
  plWorld* pWorld = GetOwner()->GetWorld();

  plGameObject* pSocket;
  if (!pWorld->TryGetObject(hSocket, pSocket))
    return;

  plPowerConnectorComponent* pConnector;
  if (pSocket->TryGetComponentOfBaseType(pConnector))
  {
    // don't connect to an already connected object
    if (pConnector->IsConnected())
      return;
  }

  const plTransform tSocket = pSocket->GetGlobalTransform();

  plGameObjectDesc go;
  go.m_hParent = hSocket;

  plGameObject* pAttach;
  m_hAttachPoint = pWorld->CreateObject(go, pAttach);

  plPhysicsWorldModuleInterface* pPhysicsWorldModule = GetWorld()->GetOrCreateModule<plPhysicsWorldModuleInterface>();

  plPhysicsWorldModuleInterface::FixedJointConfig cfg;
  cfg.m_hActorA = {};
  cfg.m_hActorB = GetOwner()->GetHandle();
  cfg.m_LocalFrameA = tSocket;
  pPhysicsWorldModule->AddFixedJointComponent(pAttach, cfg);

  SetConnectedTo(hSocket);

  if (!m_hGrabbedBy.IsInvalidated())
  {
    plGameObject* pGrab;
    if (pWorld->TryGetObject(m_hGrabbedBy, pGrab))
    {
      plMsgReleaseObjectGrab msg;
      msg.m_hGrabbedObjectToRelease = GetOwner()->GetHandle();
      pGrab->SendMessage(msg);
    }
  }
}

void plPowerConnectorComponent::Detach()
{
  if (IsConnected())
  {
    m_DetachTime = GetOwner()->GetWorld()->GetClock().GetAccumulatedTime();

    SetConnectedTo({});
  }

  if (!m_hAttachPoint.IsInvalidated())
  {
    GetOwner()->GetWorld()->DeleteObjectDelayed(m_hAttachPoint, false);
    m_hAttachPoint.Invalidate();
  }
}

void plPowerConnectorComponent::InputChanged(plUInt16 uiPrevInput, plUInt16 uiInput)
{
  if (!IsActiveAndSimulating())
    return;

  plEventMsgSetPowerInput msg;
  msg.m_uiPrevValue = uiPrevInput;
  msg.m_uiNewValue = uiInput;

  GetOwner()->PostEventMessage(msg, this, plTime());

  if (m_hBuddy.IsInvalidated())
    return;

  plGameObject* pBuddy;
  if (GetOwner()->GetWorld()->TryGetObject(m_hBuddy, pBuddy))
  {
    plPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetOutput(uiInput);
    }
  }
}

void plPowerConnectorComponent::OutputChanged(plUInt16 uiOutput)
{
  if (!IsActiveAndSimulating())
    return;

  if (m_hConnectedTo.IsInvalidated())
    return;

  plGameObject* pConnectedTo;
  if (GetOwner()->GetWorld()->TryGetObject(m_hConnectedTo, pConnectedTo))
  {
    plPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetInput(uiOutput);
    }
  }
}
