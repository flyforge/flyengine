
PLASMA_ALWAYS_INLINE plWorld* plWorldModule::GetWorld()
{
  return m_pWorld;
}

PLASMA_ALWAYS_INLINE const plWorld* plWorldModule::GetWorld() const
{
  return m_pWorld;
}

//////////////////////////////////////////////////////////////////////////

template <typename ModuleType, typename RTTIType>
plWorldModuleTypeId plWorldModuleFactory::RegisterWorldModule()
{
  struct Helper
  {
    static plWorldModule* Create(plAllocatorBase* pAllocator, plWorld* pWorld) { return PLASMA_NEW(pAllocator, ModuleType, pWorld); }
  };

  const plRTTI* pRtti = plGetStaticRTTI<RTTIType>();
  return RegisterWorldModule(pRtti, &Helper::Create);
}
