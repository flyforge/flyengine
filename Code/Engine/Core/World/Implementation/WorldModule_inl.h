
PL_ALWAYS_INLINE plWorld* plWorldModule::GetWorld()
{
  return m_pWorld;
}

PL_ALWAYS_INLINE const plWorld* plWorldModule::GetWorld() const
{
  return m_pWorld;
}

//////////////////////////////////////////////////////////////////////////

template <typename ModuleType, typename RTTIType>
plWorldModuleTypeId plWorldModuleFactory::RegisterWorldModule()
{
  struct Helper
  {
    static plWorldModule* Create(plAllocator* pAllocator, plWorld* pWorld) { return PL_NEW(pAllocator, ModuleType, pWorld); }
  };

  const plRTTI* pRtti = plGetStaticRTTI<RTTIType>();
  return RegisterWorldModule(pRtti, &Helper::Create);
}
