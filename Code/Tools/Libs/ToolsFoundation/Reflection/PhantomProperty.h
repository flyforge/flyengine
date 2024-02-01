#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class plPhantomConstantProperty : public plAbstractConstantProperty
{
public:
  plPhantomConstantProperty(const plReflectedPropertyDescriptor* pDesc);
  ~plPhantomConstantProperty();

  virtual const plRTTI* GetSpecificType() const override;
  virtual void* GetPropertyPointer() const override;
  virtual plVariant GetConstant() const override { return m_Value; }

private:
  plVariant m_Value;
  plString m_sPropertyNameStorage;
  const plRTTI* m_pPropertyType;
};

class plPhantomMemberProperty : public plAbstractMemberProperty
{
public:
  plPhantomMemberProperty(const plReflectedPropertyDescriptor* pDesc);
  ~plPhantomMemberProperty();

  virtual const plRTTI* GetSpecificType() const override;
  virtual void* GetPropertyPointer(const void* pInstance) const override { return nullptr; }
  virtual void GetValuePtr(const void* pInstance, void* pObject) const override {}
  virtual void SetValuePtr(void* pInstance, const void* pObject) const override {}

private:
  plString m_sPropertyNameStorage;
  const plRTTI* m_pPropertyType;
};

class plPhantomFunctionProperty : public plAbstractFunctionProperty
{
public:
  plPhantomFunctionProperty(plReflectedFunctionDescriptor* pDesc);
  ~plPhantomFunctionProperty();

  virtual plFunctionType::Enum GetFunctionType() const override;
  virtual const plRTTI* GetReturnType() const override;
  virtual plBitflags<plPropertyFlags> GetReturnFlags() const override;
  virtual plUInt32 GetArgumentCount() const override;
  virtual const plRTTI* GetArgumentType(plUInt32 uiParamIndex) const override;
  virtual plBitflags<plPropertyFlags> GetArgumentFlags(plUInt32 uiParamIndex) const override;
  virtual void Execute(void* pInstance, plArrayPtr<plVariant> values, plVariant& ref_returnValue) const override;

private:
  plString m_sPropertyNameStorage;
  plEnum<plFunctionType> m_FunctionType;
  plFunctionArgumentDescriptor m_ReturnValue;
  plDynamicArray<plFunctionArgumentDescriptor> m_Arguments;
};


class plPhantomArrayProperty : public plAbstractArrayProperty
{
public:
  plPhantomArrayProperty(const plReflectedPropertyDescriptor* pDesc);
  ~plPhantomArrayProperty();

  virtual const plRTTI* GetSpecificType() const override;
  virtual plUInt32 GetCount(const void* pInstance) const override { return 0; }
  virtual void GetValue(const void* pInstance, plUInt32 uiIndex, void* pObject) const override {}
  virtual void SetValue(void* pInstance, plUInt32 uiIndex, const void* pObject) const override {}
  virtual void Insert(void* pInstance, plUInt32 uiIndex, const void* pObject) const override {}
  virtual void Remove(void* pInstance, plUInt32 uiIndex) const override {}
  virtual void Clear(void* pInstance) const override {}
  virtual void SetCount(void* pInstance, plUInt32 uiCount) const override {}


private:
  plString m_sPropertyNameStorage;
  const plRTTI* m_pPropertyType;
};


class plPhantomSetProperty : public plAbstractSetProperty
{
public:
  plPhantomSetProperty(const plReflectedPropertyDescriptor* pDesc);
  ~plPhantomSetProperty();

  virtual const plRTTI* GetSpecificType() const override;
  virtual bool IsEmpty(const void* pInstance) const override { return true; }
  virtual void Clear(void* pInstance) const override {}
  virtual void Insert(void* pInstance, const void* pObject) const override {}
  virtual void Remove(void* pInstance, const void* pObject) const override {}
  virtual bool Contains(const void* pInstance, const void* pObject) const override { return false; }
  virtual void GetValues(const void* pInstance, plDynamicArray<plVariant>& out_keys) const override {}

private:
  plString m_sPropertyNameStorage;
  const plRTTI* m_pPropertyType;
};


class plPhantomMapProperty : public plAbstractMapProperty
{
public:
  plPhantomMapProperty(const plReflectedPropertyDescriptor* pDesc);
  ~plPhantomMapProperty();

  virtual const plRTTI* GetSpecificType() const override;
  virtual bool IsEmpty(const void* pInstance) const override { return true; }
  virtual void Clear(void* pInstance) const override {}
  virtual void Insert(void* pInstance, const char* szKey, const void* pObject) const override {}
  virtual void Remove(void* pInstance, const char* szKey) const override {}
  virtual bool Contains(const void* pInstance, const char* szKey) const override { return false; }
  virtual bool GetValue(const void* pInstance, const char* szKey, void* pObject) const override { return false; }
  virtual void GetKeys(const void* pInstance, plHybridArray<plString, 16>& out_keys) const override {}

private:
  plString m_sPropertyNameStorage;
  const plRTTI* m_pPropertyType;
};
