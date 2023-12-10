struct plJniModifiers
{
  enum Enum
  {
    PUBLIC = 1,
    PRIVATE = 2,
    PROTECTED = 4,
    STATIC = 8,
    FINAL = 16,
    SYNCHRONIZED = 32,
    VOLATILE = 64,
    TRANSIENT = 128,
    NATIVE = 256,
    INTERFACE = 512,
    ABSTRACT = 1024,
    STRICT = 2048,
  };
};

plJniObject::plJniObject(jobject object, plJniOwnerShip ownerShip)
  : m_class(nullptr)
{
  switch (ownerShip)
  {
    case plJniOwnerShip::OWN:
      m_object = object;
      m_own = true;
      break;

    case plJniOwnerShip::COPY:
      m_object = plJniAttachment::GetEnv()->NewLocalRef(object);
      m_own = true;
      break;

    case plJniOwnerShip::BORROW:
      m_object = object;
      m_own = false;
      break;
  }
}

plJniObject::plJniObject(const plJniObject& other)
  : m_class(nullptr)
{
  m_object = plJniAttachment::GetEnv()->NewLocalRef(other.m_object);
  m_own = true;
}

plJniObject::plJniObject(plJniObject&& other)
{
  m_object = other.m_object;
  m_class = other.m_class;
  m_own = other.m_own;

  other.m_object = nullptr;
  other.m_class = nullptr;
  other.m_own = false;
}

plJniObject& plJniObject::operator=(const plJniObject& other)
{
  if (this == &other)
    return *this;

  Reset();
  m_object = plJniAttachment::GetEnv()->NewLocalRef(other.m_object);
  m_own = true;
  return *this;
}

plJniObject& plJniObject::operator=(plJniObject&& other)
{
  if (this == &other)
    return *this;

  Reset();

  m_object = other.m_object;
  m_class = other.m_class;
  m_own = other.m_own;

  other.m_object = nullptr;
  other.m_class = nullptr;
  other.m_own = false;

  return *this;
}

plJniObject::~plJniObject()
{
  Reset();
}

void plJniObject::Reset()
{
  if (m_object && m_own)
  {
    plJniAttachment::GetEnv()->DeleteLocalRef(m_object);
    m_object = nullptr;
    m_own = false;
  }
  if (m_class)
  {
    plJniAttachment::GetEnv()->DeleteLocalRef(m_class);
    m_class = nullptr;
  }
}

jobject plJniObject::GetJObject() const
{
  return m_object;
}

bool plJniObject::operator==(const plJniObject& other) const
{
  return plJniAttachment::GetEnv()->IsSameObject(m_object, other.m_object) == JNI_TRUE;
}

bool plJniObject::operator!=(const plJniObject& other) const
{
  return !operator==(other);
}

// Template specializations to dispatch to the correct JNI method for each C++ type.
template <typename T, bool unused = false>
struct plJniTraits
{
  static_assert(unused, "The passed C++ type is not supported by the JNI wrapper. Arguments and returns types must be one of bool, signed char/jbyte, unsigned short/jchar, short/jshort, int/jint, long long/jlong, float/jfloat, double/jdouble, plJniObject, plJniString or plJniClass.");

  // Places the argument inside a jvalue union.
  static jvalue ToValue(T);

  // Retrieves the Java class static type of the argument. For primitives, this is not the boxed type, but the primitive type.
  static plJniClass GetStaticType();

  // Retrieves the Java class dynamic type of the argument. For primitives, this is not the boxed type, but the primitive type.
  static plJniClass GetRuntimeType(T);

  // Creates an invalid/null object to return in case of errors.
  static T GetEmptyObject();

  // Call an instance method with the return type.
  template <typename... Args>
  static T CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  // Call a static method with the return type.
  template <typename... Args>
  static T CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  // Sets/gets a field of the type.
  static void SetField(jobject self, jfieldID field, T);
  static T GetField(jobject self, jfieldID field);

  // Sets/gets a static field of the type.
  static void SetStaticField(jclass clazz, jfieldID field, T);
  static T GetStaticField(jclass clazz, jfieldID field);

  // Appends the JNI type signature of this type to the string buf
  static bool AppendSignature(const T& obj, plStringBuilder& str);
  static const char* GetSignatureStatic();
};

template <>
struct plJniTraits<bool>
{
  static inline jvalue ToValue(bool value);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(bool);

  static inline bool GetEmptyObject();

  template <typename... Args>
  static bool CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static bool CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, bool arg);
  static inline bool GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, bool arg);
  static inline bool GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(bool, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<jbyte>
{
  static inline jvalue ToValue(jbyte value);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(jbyte);

  static inline jbyte GetEmptyObject();

  template <typename... Args>
  static jbyte CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jbyte CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jbyte arg);
  static inline jbyte GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jbyte arg);
  static inline jbyte GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jbyte, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<jchar>
{
  static inline jvalue ToValue(jchar value);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(jchar);

  static inline jchar GetEmptyObject();

  template <typename... Args>
  static jchar CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jchar CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jchar arg);
  static inline jchar GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jchar arg);
  static inline jchar GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jchar, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<jshort>
{
  static inline jvalue ToValue(jshort value);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(jshort);

  static inline jshort GetEmptyObject();

  template <typename... Args>
  static jshort CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jshort CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jshort arg);
  static inline jshort GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jshort arg);
  static inline jshort GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jshort, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<jint>
{
  static inline jvalue ToValue(jint value);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(jint);

  static inline jint GetEmptyObject();

  template <typename... Args>
  static jint CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jint CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jint arg);
  static inline jint GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jint arg);
  static inline jint GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jint, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<jlong>
{
  static inline jvalue ToValue(jlong value);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(jlong);

  static inline jlong GetEmptyObject();

  template <typename... Args>
  static jlong CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jlong CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jlong arg);
  static inline jlong GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jlong arg);
  static inline jlong GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jlong, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<jfloat>
{
  static inline jvalue ToValue(jfloat value);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(jfloat);

  static inline jfloat GetEmptyObject();

  template <typename... Args>
  static jfloat CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jfloat CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jfloat arg);
  static inline jfloat GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jfloat arg);
  static inline jfloat GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jfloat, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<jdouble>
{
  static inline jvalue ToValue(jdouble value);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(jdouble);

  static inline jdouble GetEmptyObject();

  template <typename... Args>
  static jdouble CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jdouble CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jdouble arg);
  static inline jdouble GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jdouble arg);
  static inline jdouble GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jdouble, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<plJniObject>
{
  static inline jvalue ToValue(const plJniObject& object);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(const plJniObject& object);

  static inline plJniObject GetEmptyObject();

  template <typename... Args>
  static plJniObject CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static plJniObject CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, const plJniObject& arg);
  static inline plJniObject GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, const plJniObject& arg);
  static inline plJniObject GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(const plJniObject& obj, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<plJniClass>
{
  static inline jvalue ToValue(const plJniClass& object);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(const plJniClass& object);

  static inline plJniClass GetEmptyObject();

  template <typename... Args>
  static plJniClass CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static plJniClass CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, const plJniClass& arg);
  static inline plJniClass GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, const plJniClass& arg);
  static inline plJniClass GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(const plJniClass& obj, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<plJniString>
{
  static inline jvalue ToValue(const plJniString& object);

  static inline plJniClass GetStaticType();

  static inline plJniClass GetRuntimeType(const plJniString& object);

  static inline plJniString GetEmptyObject();

  template <typename... Args>
  static plJniString CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static plJniString CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, const plJniString& arg);
  static inline plJniString GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, const plJniString& arg);
  static inline plJniString GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(const plJniString& obj, plStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct plJniTraits<void>
{
  static inline plJniClass GetStaticType();

  static inline void GetEmptyObject();

  template <typename... Args>
  static void CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static void CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline const char* GetSignatureStatic();
};

// Helpers to unpack variadic templates.
struct plJniImpl
{
  static void CollectArgumentTypes(plJniClass* target)
  {
  }

  template <typename T, typename... Tail>
  static void CollectArgumentTypes(plJniClass* target, const T& arg, const Tail&... tail)
  {
    *target = plJniTraits<T>::GetRuntimeType(arg);
    return plJniImpl::CollectArgumentTypes(target + 1, tail...);
  }

  static void UnpackArgs(jvalue* target)
  {
  }

  template <typename T, typename... Tail>
  static void UnpackArgs(jvalue* target, const T& arg, const Tail&... tail)
  {
    *target = plJniTraits<T>::ToValue(arg);
    return UnpackArgs(target + 1, tail...);
  }

  template <typename Ret, typename... Args>
  static bool BuildMethodSignature(plStringBuilder& signature, const Args&... args)
  {
    signature.Append("(");
    if (!plJniImpl::AppendSignature(signature, args...))
    {
      return false;
    }
    signature.Append(")");
    signature.Append(plJniTraits<Ret>::GetSignatureStatic());
    return true;
  }

  static bool AppendSignature(plStringBuilder& signature)
  {
    return true;
  }

  template <typename T, typename... Tail>
  static bool AppendSignature(plStringBuilder& str, const T& arg, const Tail&... tail)
  {
    return plJniTraits<T>::AppendSignature(arg, str) && AppendSignature(str, tail...);
  }
};

jvalue plJniTraits<bool>::ToValue(bool value)
{
  jvalue result;
  result.z = value ? JNI_TRUE : JNI_FALSE;
  return result;
}

plJniClass plJniTraits<bool>::GetStaticType()
{
  return plJniClass("java/lang/Boolean").UnsafeGetStaticField<plJniClass>("TYPE", "Ljava/lang/Class;");
}

plJniClass plJniTraits<bool>::GetRuntimeType(bool)
{
  return GetStaticType();
}

bool plJniTraits<bool>::GetEmptyObject()
{
  return false;
}

template <typename... Args>
bool plJniTraits<bool>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallBooleanMethodA(self, method, array) == JNI_TRUE;
}

template <typename... Args>
bool plJniTraits<bool>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallStaticBooleanMethodA(clazz, method, array) == JNI_TRUE;
}

void plJniTraits<bool>::SetField(jobject self, jfieldID field, bool arg)
{
  return plJniAttachment::GetEnv()->SetBooleanField(self, field, arg ? JNI_TRUE : JNI_FALSE);
}

bool plJniTraits<bool>::GetField(jobject self, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetBooleanField(self, field) == JNI_TRUE;
}

void plJniTraits<bool>::SetStaticField(jclass clazz, jfieldID field, bool arg)
{
  return plJniAttachment::GetEnv()->SetStaticBooleanField(clazz, field, arg ? JNI_TRUE : JNI_FALSE);
}

bool plJniTraits<bool>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetStaticBooleanField(clazz, field) == JNI_TRUE;
}

bool plJniTraits<bool>::AppendSignature(bool, plStringBuilder& str)
{
  str.Append("Z");
  return true;
}

const char* plJniTraits<bool>::GetSignatureStatic()
{
  return "Z";
}

jvalue plJniTraits<jbyte>::ToValue(jbyte value)
{
  jvalue result;
  result.b = value;
  return result;
}

plJniClass plJniTraits<jbyte>::GetStaticType()
{
  return plJniClass("java/lang/Byte").UnsafeGetStaticField<plJniClass>("TYPE", "Ljava/lang/Class;");
}

plJniClass plJniTraits<jbyte>::GetRuntimeType(jbyte)
{
  return GetStaticType();
}

jbyte plJniTraits<jbyte>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jbyte plJniTraits<jbyte>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallByteMethodA(self, method, array);
}

template <typename... Args>
jbyte plJniTraits<jbyte>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallStaticByteMethodA(clazz, method, array);
}

void plJniTraits<jbyte>::SetField(jobject self, jfieldID field, jbyte arg)
{
  return plJniAttachment::GetEnv()->SetByteField(self, field, arg);
}

jbyte plJniTraits<jbyte>::GetField(jobject self, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetByteField(self, field);
}

void plJniTraits<jbyte>::SetStaticField(jclass clazz, jfieldID field, jbyte arg)
{
  return plJniAttachment::GetEnv()->SetStaticByteField(clazz, field, arg);
}

jbyte plJniTraits<jbyte>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetStaticByteField(clazz, field);
}

bool plJniTraits<jbyte>::AppendSignature(jbyte, plStringBuilder& str)
{
  str.Append("B");
  return true;
}

const char* plJniTraits<jbyte>::GetSignatureStatic()
{
  return "B";
}

jvalue plJniTraits<jchar>::ToValue(jchar value)
{
  jvalue result;
  result.c = value;
  return result;
}

plJniClass plJniTraits<jchar>::GetStaticType()
{
  return plJniClass("java/lang/Character").UnsafeGetStaticField<plJniClass>("TYPE", "Ljava/lang/Class;");
}

plJniClass plJniTraits<jchar>::GetRuntimeType(jchar)
{
  return GetStaticType();
}

jchar plJniTraits<jchar>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jchar plJniTraits<jchar>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallCharMethodA(self, method, array);
}

template <typename... Args>
jchar plJniTraits<jchar>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallStaticCharMethodA(clazz, method, array);
}

void plJniTraits<jchar>::SetField(jobject self, jfieldID field, jchar arg)
{
  return plJniAttachment::GetEnv()->SetCharField(self, field, arg);
}

jchar plJniTraits<jchar>::GetField(jobject self, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetCharField(self, field);
}

void plJniTraits<jchar>::SetStaticField(jclass clazz, jfieldID field, jchar arg)
{
  return plJniAttachment::GetEnv()->SetStaticCharField(clazz, field, arg);
}

jchar plJniTraits<jchar>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetStaticCharField(clazz, field);
}

bool plJniTraits<jchar>::AppendSignature(jchar, plStringBuilder& str)
{
  str.Append("C");
  return true;
}

const char* plJniTraits<jchar>::GetSignatureStatic()
{
  return "C";
}

jvalue plJniTraits<jshort>::ToValue(jshort value)
{
  jvalue result;
  result.s = value;
  return result;
}

plJniClass plJniTraits<jshort>::GetStaticType()
{
  return plJniClass("java/lang/Short").UnsafeGetStaticField<plJniClass>("TYPE", "Ljava/lang/Class;");
}

plJniClass plJniTraits<jshort>::GetRuntimeType(jshort)
{
  return GetStaticType();
}

jshort plJniTraits<jshort>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jshort plJniTraits<jshort>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallShortMethodA(self, method, array);
}

template <typename... Args>
jshort plJniTraits<jshort>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallStaticShortMethodA(clazz, method, array);
}

void plJniTraits<jshort>::SetField(jobject self, jfieldID field, jshort arg)
{
  return plJniAttachment::GetEnv()->SetShortField(self, field, arg);
}

jshort plJniTraits<jshort>::GetField(jobject self, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetShortField(self, field);
}

void plJniTraits<jshort>::SetStaticField(jclass clazz, jfieldID field, jshort arg)
{
  return plJniAttachment::GetEnv()->SetStaticShortField(clazz, field, arg);
}

jshort plJniTraits<jshort>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetStaticShortField(clazz, field);
}

bool plJniTraits<jshort>::AppendSignature(jshort, plStringBuilder& str)
{
  str.Append("S");
  return true;
}

const char* plJniTraits<jshort>::GetSignatureStatic()
{
  return "S";
}

jvalue plJniTraits<jint>::ToValue(jint value)
{
  jvalue result;
  result.i = value;
  return result;
}

plJniClass plJniTraits<jint>::GetStaticType()
{
  return plJniClass("java/lang/Integer").UnsafeGetStaticField<plJniClass>("TYPE", "Ljava/lang/Class;");
}

plJniClass plJniTraits<jint>::GetRuntimeType(jint)
{
  return GetStaticType();
}

jint plJniTraits<jint>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jint plJniTraits<jint>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallIntMethodA(self, method, array);
}

template <typename... Args>
jint plJniTraits<jint>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallStaticIntMethodA(clazz, method, array);
}

void plJniTraits<jint>::SetField(jobject self, jfieldID field, jint arg)
{
  return plJniAttachment::GetEnv()->SetIntField(self, field, arg);
}

jint plJniTraits<jint>::GetField(jobject self, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetIntField(self, field);
}

void plJniTraits<jint>::SetStaticField(jclass clazz, jfieldID field, jint arg)
{
  return plJniAttachment::GetEnv()->SetStaticIntField(clazz, field, arg);
}

jint plJniTraits<jint>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetStaticIntField(clazz, field);
}

bool plJniTraits<jint>::AppendSignature(jint, plStringBuilder& str)
{
  str.Append("I");
  return true;
}

const char* plJniTraits<jint>::GetSignatureStatic()
{
  return "I";
}

jvalue plJniTraits<jlong>::ToValue(jlong value)
{
  jvalue result;
  result.j = value;
  return result;
}

plJniClass plJniTraits<jlong>::GetStaticType()
{
  return plJniClass("java/lang/Long").UnsafeGetStaticField<plJniClass>("TYPE", "Ljava/lang/Class;");
}

plJniClass plJniTraits<jlong>::GetRuntimeType(jlong)
{
  return GetStaticType();
}

jlong plJniTraits<jlong>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jlong plJniTraits<jlong>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallLongMethodA(self, method, array);
}

template <typename... Args>
jlong plJniTraits<jlong>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallStaticLongMethodA(clazz, method, array);
}

void plJniTraits<jlong>::SetField(jobject self, jfieldID field, jlong arg)
{
  return plJniAttachment::GetEnv()->SetLongField(self, field, arg);
}

jlong plJniTraits<jlong>::GetField(jobject self, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetLongField(self, field);
}

void plJniTraits<jlong>::SetStaticField(jclass clazz, jfieldID field, jlong arg)
{
  return plJniAttachment::GetEnv()->SetStaticLongField(clazz, field, arg);
}

jlong plJniTraits<jlong>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetStaticLongField(clazz, field);
}

bool plJniTraits<jlong>::AppendSignature(jlong, plStringBuilder& str)
{
  str.Append("J");
  return true;
}

const char* plJniTraits<jlong>::GetSignatureStatic()
{
  return "J";
}

jvalue plJniTraits<jfloat>::ToValue(jfloat value)
{
  jvalue result;
  result.f = value;
  return result;
}

plJniClass plJniTraits<jfloat>::GetStaticType()
{
  return plJniClass("java/lang/Float").UnsafeGetStaticField<plJniClass>("TYPE", "Ljava/lang/Class;");
}

plJniClass plJniTraits<jfloat>::GetRuntimeType(jfloat)
{
  return GetStaticType();
}

jfloat plJniTraits<jfloat>::GetEmptyObject()
{
  return nanf("");
}

template <typename... Args>
jfloat plJniTraits<jfloat>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallFloatMethodA(self, method, array);
}

template <typename... Args>
jfloat plJniTraits<jfloat>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallStaticFloatMethodA(clazz, method, array);
}

void plJniTraits<jfloat>::SetField(jobject self, jfieldID field, jfloat arg)
{
  return plJniAttachment::GetEnv()->SetFloatField(self, field, arg);
}

jfloat plJniTraits<jfloat>::GetField(jobject self, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetFloatField(self, field);
}

void plJniTraits<jfloat>::SetStaticField(jclass clazz, jfieldID field, jfloat arg)
{
  return plJniAttachment::GetEnv()->SetStaticFloatField(clazz, field, arg);
}

jfloat plJniTraits<jfloat>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetStaticFloatField(clazz, field);
}

bool plJniTraits<jfloat>::AppendSignature(jfloat, plStringBuilder& str)
{
  str.Append("F");
  return true;
}

const char* plJniTraits<jfloat>::GetSignatureStatic()
{
  return "F";
}

jvalue plJniTraits<jdouble>::ToValue(jdouble value)
{
  jvalue result;
  result.d = value;
  return result;
}

plJniClass plJniTraits<jdouble>::GetStaticType()
{
  return plJniClass("java/lang/Double").UnsafeGetStaticField<plJniClass>("TYPE", "Ljava/lang/Class;");
}

plJniClass plJniTraits<jdouble>::GetRuntimeType(jdouble)
{
  return GetStaticType();
}

jdouble plJniTraits<jdouble>::GetEmptyObject()
{
  return nan("");
}

template <typename... Args>
jdouble plJniTraits<jdouble>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallDoubleMethodA(self, method, array);
}

template <typename... Args>
jdouble plJniTraits<jdouble>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallStaticDoubleMethodA(clazz, method, array);
}

void plJniTraits<jdouble>::SetField(jobject self, jfieldID field, jdouble arg)
{
  return plJniAttachment::GetEnv()->SetDoubleField(self, field, arg);
}

jdouble plJniTraits<jdouble>::GetField(jobject self, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetDoubleField(self, field);
}

void plJniTraits<jdouble>::SetStaticField(jclass clazz, jfieldID field, jdouble arg)
{
  return plJniAttachment::GetEnv()->SetStaticDoubleField(clazz, field, arg);
}

jdouble plJniTraits<jdouble>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniAttachment::GetEnv()->GetStaticDoubleField(clazz, field);
}

bool plJniTraits<jdouble>::AppendSignature(jdouble, plStringBuilder& str)
{
  str.Append("D");
  return true;
}

const char* plJniTraits<jdouble>::GetSignatureStatic()
{
  return "D";
}

jvalue plJniTraits<plJniObject>::ToValue(const plJniObject& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

plJniClass plJniTraits<plJniObject>::GetStaticType()
{
  return plJniClass("java/lang/Object");
}

plJniClass plJniTraits<plJniObject>::GetRuntimeType(const plJniObject& arg)
{
  return arg.GetClass();
}

plJniObject plJniTraits<plJniObject>::GetEmptyObject()
{
  return plJniObject();
}

template <typename... Args>
plJniObject plJniTraits<plJniObject>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniObject(plJniAttachment::GetEnv()->CallObjectMethodA(self, method, array), plJniOwnerShip::OWN);
}

template <typename... Args>
plJniObject plJniTraits<plJniObject>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniObject(plJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array), plJniOwnerShip::OWN);
}

void plJniTraits<plJniObject>::SetField(jobject self, jfieldID field, const plJniObject& arg)
{
  return plJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

plJniObject plJniTraits<plJniObject>::GetField(jobject self, jfieldID field)
{
  return plJniObject(plJniAttachment::GetEnv()->GetObjectField(self, field), plJniOwnerShip::OWN);
}

void plJniTraits<plJniObject>::SetStaticField(jclass clazz, jfieldID field, const plJniObject& arg)
{
  return plJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

plJniObject plJniTraits<plJniObject>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniObject(plJniAttachment::GetEnv()->GetStaticObjectField(clazz, field), plJniOwnerShip::OWN);
}

bool plJniTraits<plJniObject>::AppendSignature(const plJniObject& obj, plStringBuilder& str)
{
  if (obj.IsNull())
  {
    // Ensure null objects never generate valid signatures in order to force using the reflection path
    return false;
  }
  else
  {
    str.Append("L");
    str.Append(obj.GetClass().UnsafeCall<plJniString>("getName", "()Ljava/lang/String;").GetData());
    str.ReplaceAll(".", "/");
    str.Append(";");
    return true;
  }
}

const char* plJniTraits<plJniObject>::GetSignatureStatic()
{
  return "Ljava/lang/Object;";
}

jvalue plJniTraits<plJniClass>::ToValue(const plJniClass& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

plJniClass plJniTraits<plJniClass>::GetStaticType()
{
  return plJniClass("java/lang/Class");
}

plJniClass plJniTraits<plJniClass>::GetRuntimeType(const plJniClass& arg)
{
  // Assume there are no types derived from Class
  return GetStaticType();
}

plJniClass plJniTraits<plJniClass>::GetEmptyObject()
{
  return plJniClass();
}

template <typename... Args>
plJniClass plJniTraits<plJniClass>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniClass(jclass(plJniAttachment::GetEnv()->CallObjectMethodA(self, method, array)), plJniOwnerShip::OWN);
}

template <typename... Args>
plJniClass plJniTraits<plJniClass>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniClass(jclass(plJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array)), plJniOwnerShip::OWN);
}

void plJniTraits<plJniClass>::SetField(jobject self, jfieldID field, const plJniClass& arg)
{
  return plJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

plJniClass plJniTraits<plJniClass>::GetField(jobject self, jfieldID field)
{
  return plJniClass(jclass(plJniAttachment::GetEnv()->GetObjectField(self, field)), plJniOwnerShip::OWN);
}

void plJniTraits<plJniClass>::SetStaticField(jclass clazz, jfieldID field, const plJniClass& arg)
{
  return plJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

plJniClass plJniTraits<plJniClass>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniClass(jclass(plJniAttachment::GetEnv()->GetStaticObjectField(clazz, field)), plJniOwnerShip::OWN);
}

bool plJniTraits<plJniClass>::AppendSignature(const plJniClass& obj, plStringBuilder& str)
{
  str.Append("Ljava/lang/Class;");
  return true;
}

const char* plJniTraits<plJniClass>::GetSignatureStatic()
{
  return "Ljava/lang/Class;";
}

jvalue plJniTraits<plJniString>::ToValue(const plJniString& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

plJniClass plJniTraits<plJniString>::GetStaticType()
{
  return plJniClass("java/lang/String");
}

plJniClass plJniTraits<plJniString>::GetRuntimeType(const plJniString& arg)
{
  // Assume there are no types derived from String
  return GetStaticType();
}

plJniString plJniTraits<plJniString>::GetEmptyObject()
{
  return plJniString();
}

template <typename... Args>
plJniString plJniTraits<plJniString>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniString(jstring(plJniAttachment::GetEnv()->CallObjectMethodA(self, method, array)), plJniOwnerShip::OWN);
}

template <typename... Args>
plJniString plJniTraits<plJniString>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniString(jstring(plJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array)), plJniOwnerShip::OWN);
}

void plJniTraits<plJniString>::SetField(jobject self, jfieldID field, const plJniString& arg)
{
  return plJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

plJniString plJniTraits<plJniString>::GetField(jobject self, jfieldID field)
{
  return plJniString(jstring(plJniAttachment::GetEnv()->GetObjectField(self, field)), plJniOwnerShip::OWN);
}

void plJniTraits<plJniString>::SetStaticField(jclass clazz, jfieldID field, const plJniString& arg)
{
  return plJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

plJniString plJniTraits<plJniString>::GetStaticField(jclass clazz, jfieldID field)
{
  return plJniString(jstring(plJniAttachment::GetEnv()->GetStaticObjectField(clazz, field)), plJniOwnerShip::OWN);
}

bool plJniTraits<plJniString>::AppendSignature(const plJniString& obj, plStringBuilder& str)
{
  str.Append("Ljava/lang/String;");
  return true;
}

const char* plJniTraits<plJniString>::GetSignatureStatic()
{
  return "Ljava/lang/String;";
}

plJniClass plJniTraits<void>::GetStaticType()
{
  return plJniClass("java/lang/Void").UnsafeGetStaticField<plJniClass>("TYPE", "Ljava/lang/Class;");
}

void plJniTraits<void>::GetEmptyObject()
{
  return;
}

template <typename... Args>
void plJniTraits<void>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallVoidMethodA(self, method, array);
}

template <typename... Args>
void plJniTraits<void>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniAttachment::GetEnv()->CallStaticVoidMethodA(clazz, method, array);
}

const char* plJniTraits<void>::GetSignatureStatic()
{
  return "V";
}

template <typename... Args>
plJniObject plJniClass::CreateInstance(const Args&... args) const
{
  if (plJniAttachment::FailOnPendingErrorOrException())
  {
    return plJniObject();
  }

  const size_t N = sizeof...(args);

  plJniClass inputTypes[N];
  plJniImpl::CollectArgumentTypes(inputTypes, args...);

  plJniObject foundMethod = FindConstructor(*this, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return plJniObject();
  }

  jmethodID method = plJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.GetHandle());

  jvalue array[sizeof...(args)];
  plJniImpl::UnpackArgs(array, args...);
  return plJniObject(plJniAttachment::GetEnv()->NewObjectA(GetHandle(), method, array), plJniOwnerShip::OWN);
}

template <typename Ret, typename... Args>
Ret plJniClass::CallStatic(const char* name, const Args&... args) const
{
  if (plJniAttachment::FailOnPendingErrorOrException())
  {
    return plJniTraits<Ret>::GetEmptyObject();
  }

  if (!GetJObject())
  {
    plLog::Error("Attempting to call static method '{}' on null class.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  plStringBuilder signature;
  if (plJniImpl::BuildMethodSignature<Ret>(signature, args...))
  {
    jmethodID method = plJniAttachment::GetEnv()->GetStaticMethodID(GetHandle(), name, signature.GetData());

    if (method)
    {
      return plJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
    }
    else
    {
      plJniAttachment::GetEnv()->ExceptionClear();
    }
  }

  const size_t N = sizeof...(args);

  plJniClass returnType = plJniTraits<Ret>::GetStaticType();

  plJniClass inputTypes[N];
  plJniImpl::CollectArgumentTypes(inputTypes, args...);

  plJniObject foundMethod = FindMethod(true, name, *this, returnType, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return plJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = plJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.GetHandle());
  return plJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
}

template <typename Ret, typename... Args>
Ret plJniClass::UnsafeCallStatic(const char* name, const char* signature, const Args&... args) const
{
  if (!GetJObject())
  {
    plLog::Error("Attempting to call static method '{}' on null class.", name);
    plLog::Error("Attempting to call static method '{}' on null class.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = plJniAttachment::GetEnv()->GetStaticMethodID(GetHandle(), name, signature);
  if (!method)
  {
    plLog::Error("No such static method: '{}' with signature '{}' in class '{}'.", name, signature, ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_METHOD);
    return plJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return plJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
  }
}

template <typename Ret>
Ret plJniClass::GetStaticField(const char* name) const
{
  if (plJniAttachment::FailOnPendingErrorOrException())
  {
    return plJniTraits<Ret>::GetEmptyObject();
  }

  if (!GetJObject())
  {
    plLog::Error("Attempting to get static field '{}' on null class.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID fieldID = plJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, plJniTraits<Ret>::GetSignatureStatic());
  if (fieldID)
  {
    return plJniTraits<Ret>::GetStaticField(GetHandle(), fieldID);
  }
  else
  {
    plJniAttachment::GetEnv()->ExceptionClear();
  }

  plJniObject field = UnsafeCall<plJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", plJniString(name));

  if (plJniAttachment::GetEnv()->ExceptionOccurred())
  {
    plJniAttachment::GetEnv()->ExceptionClear();

    plLog::Error("No field named '{}' found in class '{}'.", name, ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);

    return plJniTraits<Ret>::GetEmptyObject();
  }

  if ((field.UnsafeCall<jint>("getModifiers", "()I") & plJniModifiers::STATIC) == 0)
  {
    plLog::Error("Field named '{}' in class '{}' isn't static.", name, ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  plJniClass fieldType = field.UnsafeCall<plJniClass>("getType", "()Ljava/lang/Class;");

  plJniClass returnType = plJniTraits<Ret>::GetStaticType();

  if (!returnType.IsAssignableFrom(fieldType))
  {
    plLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned to return type '{}'.", name, fieldType.ToString().GetData(), ToString().GetData(), returnType.ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  return plJniTraits<Ret>::GetStaticField(GetHandle(), plJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()));
}

template <typename Ret>
Ret plJniClass::UnsafeGetStaticField(const char* name, const char* signature) const
{
  if (!GetJObject())
  {
    plLog::Error("Attempting to get static field '{}' on null class.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID field = plJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, signature);
  if (!field)
  {
    plLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return plJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return plJniTraits<Ret>::GetStaticField(GetHandle(), field);
  }
}

template <typename T>
void plJniClass::SetStaticField(const char* name, const T& arg) const
{
  if (plJniAttachment::FailOnPendingErrorOrException())
  {
    return;
  }

  if (!GetJObject())
  {
    plLog::Error("Attempting to set static field '{}' on null class.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  plJniObject field = UnsafeCall<plJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", plJniString(name));

  if (plJniAttachment::GetEnv()->ExceptionOccurred())
  {
    plJniAttachment::GetEnv()->ExceptionClear();

    plLog::Error("No field named '{}' found in class '{}'.", name, ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);

    return;
  }

  plJniClass modifierClass("java/lang/reflect/Modifier");
  jint modifiers = field.UnsafeCall<jint>("getModifiers", "()I");

  if ((modifiers & plJniModifiers::STATIC) == 0)
  {
    plLog::Error("Field named '{}' in class '{}' isn't static.", name, ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  if ((modifiers & plJniModifiers::FINAL) != 0)
  {
    plLog::Error("Field named '{}' in class '{}' is final.", name, ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  plJniClass fieldType = field.UnsafeCall<plJniClass>("getType", "()Ljava/lang/Class;");

  plJniClass argType = plJniTraits<T>::GetRuntimeType(arg);

  if (argType.IsNull())
  {
    if (fieldType.IsPrimitive())
    {
      plLog::Error("Field '{}' of type '{}' can't be assigned null because it is a primitive type.", name, fieldType.ToString().GetData());
      plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }
  else
  {
    if (!fieldType.IsAssignableFrom(argType))
    {
      plLog::Error("Field '{}' of type '{}' can't be assigned from type '{}'.", name, fieldType.ToString().GetData(), argType.ToString().GetData());
      plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }

  return plJniTraits<T>::SetStaticField(GetHandle(), plJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()), arg);
}

template <typename T>
void plJniClass::UnsafeSetStaticField(const char* name, const char* signature, const T& arg) const
{
  if (!GetJObject())
  {
    plLog::Error("Attempting to set static field '{}' on null class.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = plJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, signature);
  if (!field)
  {
    plLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return plJniTraits<T>::SetStaticField(GetHandle(), field, arg);
  }
}

template <typename Ret, typename... Args>
Ret plJniObject::Call(const char* name, const Args&... args) const
{
  if (plJniAttachment::FailOnPendingErrorOrException())
  {
    return plJniTraits<Ret>::GetEmptyObject();
  }

  if (!m_object)
  {
    plLog::Error("Attempting to call method '{}' on null object.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  // Fast path: Lookup method via signature built from parameters.
  // This only works for exact matches, but is roughly 50 times faster.
  plStringBuilder signature;
  if (plJniImpl::BuildMethodSignature<Ret>(signature, args...))
  {
    jmethodID method = plJniAttachment::GetEnv()->GetMethodID(reinterpret_cast<jclass>(GetClass().GetHandle()), name, signature.GetData());

    if (method)
    {
      return plJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
    }
    else
    {
      plJniAttachment::GetEnv()->ExceptionClear();
    }
  }

  // Fallback to slow path using reflection
  const size_t N = sizeof...(args);

  plJniClass returnType = plJniTraits<Ret>::GetStaticType();

  plJniClass inputTypes[N];
  plJniImpl::CollectArgumentTypes(inputTypes, args...);

  plJniObject foundMethod = FindMethod(false, name, GetClass(), returnType, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return plJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = plJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.m_object);
  return plJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
}

template <typename Ret, typename... Args>
Ret plJniObject::UnsafeCall(const char* name, const char* signature, const Args&... args) const
{
  if (!m_object)
  {
    plLog::Error("Attempting to call method '{}' on null object.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = plJniAttachment::GetEnv()->GetMethodID(jclass(GetClass().m_object), name, signature);
  if (!method)
  {
    plLog::Error("No such method: '{}' with signature '{}' in class '{}'.", name, signature, GetClass().ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_METHOD);
    return plJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return plJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
  }
}

template <typename T>
void plJniObject::SetField(const char* name, const T& arg) const
{
  if (plJniAttachment::FailOnPendingErrorOrException())
  {
    return;
  }

  if (!m_object)
  {
    plLog::Error("Attempting to set field '{}' on null object.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  // No fast path here since we need to be able to report failures when attempting
  // to set final fields, which we can only do using reflection.

  plJniObject field = GetClass().UnsafeCall<plJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", plJniString(name));

  if (plJniAttachment::GetEnv()->ExceptionOccurred())
  {
    plJniAttachment::GetEnv()->ExceptionClear();

    plLog::Error("No field named '{}' found.", name);
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);

    return;
  }

  plJniClass modifierClass("java/lang/reflect/Modifier");
  jint modifiers = field.UnsafeCall<jint>("getModifiers", "()I");

  if ((modifiers & plJniModifiers::STATIC) != 0)
  {
    plLog::Error("Field named '{}' in class '{}' is static.", name, GetClass().ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  if ((modifiers & plJniModifiers::FINAL) != 0)
  {
    plLog::Error("Field named '{}' in class '{}' is final.", name, GetClass().ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  plJniClass fieldType = field.UnsafeCall<plJniClass>("getType", "()Ljava/lang/Class;");

  plJniClass argType = plJniTraits<T>::GetRuntimeType(arg);

  if (argType.IsNull())
  {
    if (fieldType.IsPrimitive())
    {
      plLog::Error("Field '{}' of type '{}'  in class '{}' can't be assigned null because it is a primitive type.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData());
      plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }
  else
  {
    if (!fieldType.IsAssignableFrom(argType))
    {
      plLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned from type '{}'.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData(), argType.ToString().GetData());
      plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }

  return plJniTraits<T>::SetField(m_object, plJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()), arg);
}

template <typename T>
void plJniObject::UnsafeSetField(const char* name, const char* signature, const T& arg) const
{
  if (!m_object)
  {
    plLog::Error("Attempting to set field '{}' on null class.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = plJniAttachment::GetEnv()->GetFieldID(jclass(GetClass().GetHandle()), name, signature);
  if (!field)
  {
    plLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return plJniTraits<T>::SetField(m_object, field, arg);
  }
}

template <typename Ret>
Ret plJniObject::GetField(const char* name) const
{
  if (plJniAttachment::FailOnPendingErrorOrException())
  {
    return plJniTraits<Ret>::GetEmptyObject();
  }

  if (!m_object)
  {
    plLog::Error("Attempting to get field '{}' on null object.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID fieldID = plJniAttachment::GetEnv()->GetFieldID(GetClass().GetHandle(), name, plJniTraits<Ret>::GetSignatureStatic());
  if (fieldID)
  {
    return plJniTraits<Ret>::GetField(m_object, fieldID);
  }
  else
  {
    plJniAttachment::GetEnv()->ExceptionClear();
  }

  plJniObject field = GetClass().UnsafeCall<plJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", plJniString(name));

  if (plJniAttachment::GetEnv()->ExceptionOccurred())
  {
    plJniAttachment::GetEnv()->ExceptionClear();

    plLog::Error("No field named '{}' found in class '{}'.", name, GetClass().ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);

    return plJniTraits<Ret>::GetEmptyObject();
  }

  if ((field.UnsafeCall<jint>("getModifiers", "()I") & plJniModifiers::STATIC) != 0)
  {
    plLog::Error("Field named '{}' in class '{}' is static.", name, GetClass().ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  plJniClass fieldType = field.UnsafeCall<plJniClass>("getType", "()Ljava/lang/Class;");

  plJniClass returnType = plJniTraits<Ret>::GetStaticType();

  if (!returnType.IsAssignableFrom(fieldType))
  {
    plLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned to return type '{}'.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData(), returnType.ToString().GetData());
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return plJniTraits<Ret>::GetEmptyObject();
  }

  return plJniTraits<Ret>::GetField(m_object, plJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()));
}

template <typename Ret>
Ret plJniObject::UnsafeGetField(const char* name, const char* signature) const
{
  if (!m_object)
  {
    plLog::Error("Attempting to get field '{}' on null class.", name);
    plJniAttachment::SetLastError(plJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = plJniAttachment::GetEnv()->GetFieldID(GetClass().GetHandle(), name, signature);
  if (!field)
  {
    plLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    plJniAttachment::SetLastError(plJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return plJniTraits<Ret>::GetField(m_object, field);
  }
}
