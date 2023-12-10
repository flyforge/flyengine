#include <Foundation/FoundationPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidJni.h>
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

thread_local JNIEnv* plJniAttachment::s_env;
thread_local bool plJniAttachment::s_ownsEnv;
thread_local int plJniAttachment::s_attachCount;
thread_local plJniErrorState plJniAttachment::s_lastError;

plJniAttachment::plJniAttachment()
{
  if (s_attachCount > 0)
  {
    s_env->PushLocalFrame(16);
  }
  else
  {
    JNIEnv* env = nullptr;
    jint envStatus = plAndroidUtils::GetAndroidJavaVM()->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    bool ownsEnv = (envStatus != JNI_OK);
    if (ownsEnv)
    {
      // Assign name to attachment since ART complains about it not being set.
      JavaVMAttachArgs args = {JNI_VERSION_1_6, "PLASMA JNI", nullptr};
      plAndroidUtils::GetAndroidJavaVM()->AttachCurrentThread(&env, &args);
    }
    else
    {
      // Assume already existing JNI environment will be alive as long as this object exists.
      PLASMA_ASSERT_DEV(env != nullptr, "");
      env->PushLocalFrame(16);
    }

    s_env = env;
    s_ownsEnv = ownsEnv;
  }

  s_attachCount++;
}

plJniAttachment::~plJniAttachment()
{
  s_attachCount--;

  if (s_attachCount == 0)
  {
    ClearLastError();

    if (s_ownsEnv)
    {
      plAndroidUtils::GetAndroidJavaVM()->DetachCurrentThread();
    }
    else
    {
      s_env->PopLocalFrame(nullptr);
    }

    s_env = nullptr;
    s_ownsEnv = false;
  }
  else
  {
    s_env->PopLocalFrame(nullptr);
  }
}

plJniObject plJniAttachment::GetActivity()
{
  return plJniObject(plAndroidUtils::GetAndroidNativeActivity(), plJniOwnerShip::BORROW);
}

JNIEnv* plJniAttachment::GetEnv()
{
  PLASMA_ASSERT_DEV(s_env != nullptr, "Thread not attached to the JVM - you forgot to create an instance of plJniAttachment in the current scope.");

#  if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  void* unused;
  PLASMA_ASSERT_DEBUG(plAndroidUtils::GetAndroidJavaVM()->GetEnv(&unused, JNI_VERSION_1_6) == JNI_OK,
    "Current thread has lost its attachment to the JVM - some OS calls can cause this to happen. Try to reduce the attachment to a smaller scope.");
#  endif

  return s_env;
}

plJniErrorState plJniAttachment::GetLastError()
{
  plJniErrorState state = s_lastError;
  return state;
}

void plJniAttachment::ClearLastError()
{
  s_lastError = plJniErrorState::SUCCESS;
}

void plJniAttachment::SetLastError(plJniErrorState state)
{
  s_lastError = state;
}

bool plJniAttachment::HasPendingException()
{
  return GetEnv()->ExceptionCheck();
}

void plJniAttachment::ClearPendingException()
{
  return GetEnv()->ExceptionClear();
}

plJniObject plJniAttachment::GetPendingException()
{
  return plJniObject(GetEnv()->ExceptionOccurred(), plJniOwnerShip::OWN);
}

bool plJniAttachment::FailOnPendingErrorOrException()
{
  if (plJniAttachment::GetLastError() != plJniErrorState::SUCCESS)
  {
    plLog::Error("Aborting call because the previous error state was not cleared.");
    return true;
  }

  if (plJniAttachment::HasPendingException())
  {
    plLog::Error("Aborting call because a Java exception is still pending.");
    plJniAttachment::SetLastError(plJniErrorState::PENDING_EXCEPTION);
    return true;
  }

  return false;
}

void plJniObject::DumpTypes(const plJniClass* inputTypes, int N, const plJniClass* returnType)
{
  if (returnType != nullptr)
  {
    plLog::Error("  With requested return type '{}'", returnType->ToString().GetData());
  }

  for (int paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    plLog::Error("  With passed param type #{} '{}'", paramIdx, inputTypes[paramIdx].IsNull() ? "(null)" : inputTypes[paramIdx].ToString().GetData());
  }
}

int plJniObject::CompareMethodSpecificity(const plJniObject& method1, const plJniObject& method2)
{
  plJniClass returnType1 = method1.UnsafeCall<plJniClass>("getReturnType", "()Ljava/lang/Class;");
  plJniClass returnType2 = method2.UnsafeCall<plJniClass>("getReturnType", "()Ljava/lang/Class;");

  plJniObject paramTypes1 = method1.UnsafeCall<plJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  plJniObject paramTypes2 = method2.UnsafeCall<plJniObject>("getParameterTypes", "()[Ljava/lang/Class;");

  jsize N = plJniAttachment::GetEnv()->GetArrayLength(jarray(paramTypes1.m_object));

  int decision = returnType1.IsAssignableFrom(returnType2) - returnType2.IsAssignableFrom(returnType1);

  for (jsize paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    plJniClass paramType1(
      jclass(plJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes1.m_object), paramIdx)), plJniOwnerShip::OWN);
    plJniClass paramType2(
      jclass(plJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes2.m_object), paramIdx)), plJniOwnerShip::OWN);

    int paramDecision = paramType1.IsAssignableFrom(paramType2) - paramType2.IsAssignableFrom(paramType1);

    if (decision == 0)
    {
      // No method is more specific yet
      decision = paramDecision;
    }
    else if (paramDecision != 0 && decision != paramDecision)
    {
      // There is no clear specificity ordering - one type is more specific, but the other less so
      return 0;
    }
  }

  return decision;
}

bool plJniObject::IsMethodViable(bool bStatic, const plJniObject& candidateMethod, const plJniClass& returnType, plJniClass* inputTypes, int N)
{
  // Check if staticness matches
  if (plJniClass("java/lang/reflect/Modifier").UnsafeCallStatic<bool>("isStatic", "(I)Z", candidateMethod.UnsafeCall<int>("getModifiers", "()I")) !=
      bStatic)
  {
    return false;
  }

  // Check if return type is assignable to the requested type
  plJniClass candidateReturnType = candidateMethod.UnsafeCall<plJniClass>("getReturnType", "()Ljava/lang/Class;");
  if (!returnType.IsAssignableFrom(candidateReturnType))
  {
    return false;
  }

  // Check number of parameters
  plJniObject parameterTypes = candidateMethod.UnsafeCall<plJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  jsize numCandidateParams = plJniAttachment::GetEnv()->GetArrayLength(jarray(parameterTypes.m_object));
  if (numCandidateParams != N)
  {
    return false;
  }

  // Check if input parameter types are assignable to the actual parameter types
  for (jsize paramIdx = 0; paramIdx < numCandidateParams; ++paramIdx)
  {
    plJniClass paramType(
      jclass(plJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(parameterTypes.m_object), paramIdx)), plJniOwnerShip::OWN);

    if (inputTypes[paramIdx].IsNull())
    {
      if (paramType.IsPrimitive())
      {
        return false;
      }
    }
    else
    {
      if (!paramType.IsAssignableFrom(inputTypes[paramIdx]))
      {
        return false;
      }
    }
  }

  return true;
}

plJniObject plJniObject::FindMethod(
  bool bStatic, const char* name, const plJniClass& searchClass, const plJniClass& returnType, plJniClass* inputTypes, int N)
{
  if (searchClass.IsNull())
  {
    plLog::Error("Attempting to find constructor for null type.");
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return plJniObject();
  }

  plHybridArray<plJniObject, 32> bestCandidates;

  // In case of no parameters, fetch the method directly.
  if (N == 0)
  {
    plJniObject candidateMethod = searchClass.UnsafeCall<plJniObject>(
      "getMethod", "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;", plJniString(name), plJniObject());

    if (!plJniAttachment::GetEnv()->ExceptionCheck() && IsMethodViable(bStatic, candidateMethod, returnType, inputTypes, N))
    {
      bestCandidates.PushBack(candidateMethod);
    }
    else
    {
      plJniAttachment::GetEnv()->ExceptionClear();
    }
  }
  else
  {
    // For methods with parameters, loop over all methods to find one with the correct name and matching parameter types

    plJniObject methodArray = searchClass.UnsafeCall<plJniObject>("getMethods", "()[Ljava/lang/reflect/Method;");

    jsize numMethods = plJniAttachment::GetEnv()->GetArrayLength(jarray(methodArray.m_object));
    for (jsize methodIdx = 0; methodIdx < numMethods; ++methodIdx)
    {
      plJniObject candidateMethod(
        plJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(methodArray.m_object), methodIdx), plJniOwnerShip::OWN);

      plJniString methodName = candidateMethod.UnsafeCall<plJniString>("getName", "()Ljava/lang/String;");

      if (strcmp(name, methodName.GetData()) != 0)
      {
        continue;
      }

      if (!IsMethodViable(bStatic, candidateMethod, returnType, inputTypes, N))
      {
        continue;
      }

      bool isMoreSpecific = true;
      for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
      {
        int comparison = CompareMethodSpecificity(bestCandidates[candidateIdx], candidateMethod);

        if (comparison == 1)
        {
          // Remove less specific candidate and continue looping
          bestCandidates.RemoveAtAndSwap(candidateIdx);
          candidateIdx--;
        }
        else if (comparison == -1)
        {
          // We're less specific, so by transitivity there are no other methods less specific than ours that we could throw out,
          // and we can abort the loop
          isMoreSpecific = false;
          break;
        }
        else
        {
          // No relation, so do nothing
        }
      }

      if (isMoreSpecific)
      {
        bestCandidates.PushBack(candidateMethod);
      }
    }
  }

  if (bestCandidates.GetCount() == 1)
  {
    return bestCandidates[0];
  }
  else if (bestCandidates.GetCount() == 0)
  {
    plLog::Error("Overload resolution failed: No method '{}' in class '{}' matches the requested return and parameter types.", name,
      searchClass.ToString().GetData());
    DumpTypes(inputTypes, N, &returnType);
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_METHOD);
    return plJniObject();
  }
  else
  {
    plLog::Error("Overload resolution failed: Call to '{}' in class '{}' is ambiguous. Cannot decide between the following candidates:", name,
      searchClass.ToString().GetData());
    for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
    {
      plLog::Error("  Candidate #{}: '{}'", candidateIdx, bestCandidates[candidateIdx].ToString().GetData());
    }
    DumpTypes(inputTypes, N, &returnType);
    plJniAttachment::SetLastError(plJniErrorState::AMBIGUOUS_CALL);
    return plJniObject();
  }
}

int plJniObject::CompareConstructorSpecificity(const plJniObject& method1, const plJniObject& method2)
{
  plJniObject paramTypes1 = method1.UnsafeCall<plJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  plJniObject paramTypes2 = method2.UnsafeCall<plJniObject>("getParameterTypes", "()[Ljava/lang/Class;");

  jsize N = plJniAttachment::GetEnv()->GetArrayLength(jarray(paramTypes1.m_object));

  int decision = 0;

  for (jsize paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    plJniClass paramType1(
      jclass(plJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes1.m_object), paramIdx)), plJniOwnerShip::OWN);
    plJniClass paramType2(
      jclass(plJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes2.m_object), paramIdx)), plJniOwnerShip::OWN);

    int paramDecision = paramType1.IsAssignableFrom(paramType2) - paramType2.IsAssignableFrom(paramType1);

    if (decision == 0)
    {
      // No method is more specific yet
      decision = paramDecision;
    }
    else if (paramDecision != 0 && decision != paramDecision)
    {
      // There is no clear specificity ordering - one type is more specific, but the other less so
      return 0;
    }
  }

  return decision;
}

bool plJniObject::IsConstructorViable(const plJniObject& candidateMethod, plJniClass* inputTypes, int N)
{
  // Check number of parameters
  plJniObject parameterTypes = candidateMethod.UnsafeCall<plJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  jsize numCandidateParams = plJniAttachment::GetEnv()->GetArrayLength(jarray(parameterTypes.m_object));
  if (numCandidateParams != N)
  {
    return false;
  }

  // Check if input parameter types are assignable to the actual parameter types
  for (jsize paramIdx = 0; paramIdx < numCandidateParams; ++paramIdx)
  {
    plJniClass paramType(
      jclass(plJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(parameterTypes.m_object), paramIdx)), plJniOwnerShip::OWN);

    if (inputTypes[paramIdx].IsNull())
    {
      if (paramType.IsPrimitive())
      {
        return false;
      }
    }
    else
    {
      if (!paramType.IsAssignableFrom(inputTypes[paramIdx]))
      {
        return false;
      }
    }
  }

  return true;
}

plJniObject plJniObject::FindConstructor(const plJniClass& type, plJniClass* inputTypes, int N)
{
  if (type.IsNull())
  {
    plLog::Error("Attempting to find constructor for null type.");
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return plJniObject();
  }

  plHybridArray<plJniObject, 32> bestCandidates;

  // In case of no parameters, fetch the method directly.
  if (N == 0)
  {
    plJniObject candidateMethod =
      type.UnsafeCall<plJniObject>("getConstructor", "([Ljava/lang/Class;)Ljava/lang/reflect/Constructor;", plJniObject());

    if (!plJniAttachment::GetEnv()->ExceptionCheck() && IsConstructorViable(candidateMethod, inputTypes, N))
    {
      bestCandidates.PushBack(candidateMethod);
    }
    else
    {
      plJniAttachment::GetEnv()->ExceptionClear();
    }
  }
  else
  {
    // For methods with parameters, loop over all methods to find one with the correct name and matching parameter types

    plJniObject methodArray = type.UnsafeCall<plJniObject>("getConstructors", "()[Ljava/lang/reflect/Constructor;");

    jsize numMethods = plJniAttachment::GetEnv()->GetArrayLength(jarray(methodArray.m_object));
    for (jsize methodIdx = 0; methodIdx < numMethods; ++methodIdx)
    {
      plJniObject candidateMethod(
        plJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(methodArray.m_object), methodIdx), plJniOwnerShip::OWN);

      if (!IsConstructorViable(candidateMethod, inputTypes, N))
      {
        continue;
      }

      bool isMoreSpecific = true;
      for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
      {
        int comparison = CompareConstructorSpecificity(bestCandidates[candidateIdx], candidateMethod);

        if (comparison == 1)
        {
          // Remove less specific candidate and continue looping
          bestCandidates.RemoveAtAndSwap(candidateIdx);
          candidateIdx--;
        }
        else if (comparison == -1)
        {
          // We're less specific, so by transitivity there are no other methods less specific than ours that we could throw out,
          // and we can abort the loop
          isMoreSpecific = false;
          break;
        }
        else
        {
          // No relation, so do nothing
        }
      }

      if (isMoreSpecific)
      {
        bestCandidates.PushBack(candidateMethod);
      }
    }
  }

  if (bestCandidates.GetCount() == 1)
  {
    return bestCandidates[0];
  }
  else if (bestCandidates.GetCount() == 0)
  {
    plLog::Error("Overload resolution failed: No constructor in class '{}' matches the requested parameter types.", type.ToString().GetData());
    DumpTypes(inputTypes, N, nullptr);
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_METHOD);
    return plJniObject();
  }
  else
  {
    plLog::Error("Overload resolution failed: Call to constructor in class '{}' is ambiguous. Cannot decide between the following candidates:",
      type.ToString().GetData());
    for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
    {
      plLog::Error("  Candidate #{}: '{}'", candidateIdx, bestCandidates[candidateIdx].ToString().GetData());
    }
    DumpTypes(inputTypes, N, nullptr);
    plJniAttachment::SetLastError(plJniErrorState::AMBIGUOUS_CALL);
    return plJniObject();
  }
}

plJniObject::plJniObject()
  : m_object(nullptr)
  , m_class(nullptr)
  , m_own(false)
{
}

jobject plJniObject::GetHandle() const
{
  return m_object;
}

plJniClass plJniObject::GetClass() const
{
  if (!m_object)
  {
    return plJniClass();
  }

  if (!m_class)
  {
    const_cast<plJniObject*>(this)->m_class = plJniAttachment::GetEnv()->GetObjectClass(m_object);
  }

  return plJniClass(m_class, plJniOwnerShip::BORROW);
}

plJniString plJniObject::ToString() const
{
  if (plJniAttachment::FailOnPendingErrorOrException())
  {
    return plJniString();
  }

  // Implement ToString without UnsafeCall, since UnsafeCall requires ToString for diagnostic output.
  if (IsNull())
  {
    plLog::Error("Attempting to call method 'toString' on null object.");
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return plJniString();
  }

  jmethodID method = plJniAttachment::GetEnv()->GetMethodID(jclass(GetClass().m_object), "toString", "()Ljava/lang/String;");
  PLASMA_ASSERT_DEV(method, "Could not find JNI method toString()");

  return plJniTraits<plJniString>::CallInstanceMethod(m_object, method);
}

bool plJniObject::IsInstanceOf(const plJniClass& clazz) const
{
  if (IsNull())
  {
    return false;
  }

  return clazz.IsAssignableFrom(GetClass());
}

plJniString::plJniString()
  : plJniObject()
  , m_utf(nullptr)
{
}

plJniString::plJniString(const char* str)
  : plJniObject(plJniAttachment::GetEnv()->NewStringUTF(str), plJniOwnerShip::OWN)
  , m_utf(nullptr)
{
}

plJniString::plJniString(jstring string, plJniOwnerShip ownerShip)
  : plJniObject(string, ownerShip)
  , m_utf(nullptr)
{
}

plJniString::plJniString(const plJniString& other)
  : plJniObject(other)
  , m_utf(nullptr)
{
}

plJniString::plJniString(plJniString&& other)
  : plJniObject(other)
  , m_utf(nullptr)
{
  m_utf = other.m_utf;
  other.m_utf = nullptr;
}

plJniString& plJniString::operator=(const plJniString& other)
{
  if (m_utf)
  {
    plJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }

  plJniObject::operator=(other);

  return *this;
}

plJniString& plJniString::operator=(plJniString&& other)
{
  if (m_utf)
  {
    plJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }

  plJniObject::operator=(other);

  m_utf = other.m_utf;
  other.m_utf = nullptr;

  return *this;
}

plJniString::~plJniString()
{
  if (m_utf)
  {
    plJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }
}

const char* plJniString::GetData() const
{
  if (IsNull())
  {
    plLog::Error("Calling AsChar() on null Java String");
    return "<null>";
  }

  if (!m_utf)
  {
    const_cast<plJniString*>(this)->m_utf = plJniAttachment::GetEnv()->GetStringUTFChars(jstring(GetJObject()), nullptr);
  }

  return m_utf;
}


plJniClass::plJniClass()
  : plJniObject()
{
}

plJniClass::plJniClass(const char* className)
  : plJniObject(plJniAttachment::GetEnv()->FindClass(className), plJniOwnerShip::OWN)
{
  if (IsNull())
  {
    plLog::Error("Class '{}' not found.", className);
    plJniAttachment::SetLastError(plJniErrorState::CLASS_NOT_FOUND);
  }
}

plJniClass::plJniClass(jclass clazz, plJniOwnerShip ownerShip)
  : plJniObject(clazz, ownerShip)
{
}

plJniClass::plJniClass(const plJniClass& other)
  : plJniObject(static_cast<const plJniObject&>(other))
{
}

plJniClass::plJniClass(plJniClass&& other)
  : plJniObject(other)
{
}

plJniClass& plJniClass::operator=(const plJniClass& other)
{
  plJniObject::operator=(other);
  return *this;
}

plJniClass& plJniClass::operator=(plJniClass&& other)
{
  plJniObject::operator=(other);
  return *this;
}

jclass plJniClass::GetHandle() const
{
  return static_cast<jclass>(GetJObject());
}

bool plJniClass::IsAssignableFrom(const plJniClass& other) const
{
  static bool checkedApiOrder = false;
  static bool reverseArgs = false;

  JNIEnv* env = plJniAttachment::GetEnv();

  // Guard against JNI bug reversing order of arguments - fixed in
  // https://android.googlesource.com/platform/art/+/1268b742c8cff7318dc0b5b283cbaeabfe0725ba
  if (!checkedApiOrder)
  {
    plJniClass objectClass("java/lang/Object");
    plJniClass stringClass("java/lang/String");

    if (env->IsAssignableFrom(jclass(objectClass.GetJObject()), jclass(stringClass.GetJObject())))
    {
      reverseArgs = true;
    }
    checkedApiOrder = true;
  }

  if (!reverseArgs)
  {
    return env->IsAssignableFrom(jclass(other.GetJObject()), jclass(GetJObject()));
  }
  else
  {
    return env->IsAssignableFrom(jclass(GetJObject()), jclass(other.GetJObject()));
  }
}

bool plJniClass::IsPrimitive()
{
  return UnsafeCall<bool>("isPrimitive", "()Z");
}
#endif


PLASMA_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Android_AndroidJni);
