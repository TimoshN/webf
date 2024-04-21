/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_SCRIPT_WRAPPABLE_H
#define WEBF_SCRIPT_WRAPPABLE_H

//#include "build/build_config.h"
//#include "third_party/blink/renderer/platform/bindings/name_client.h"
//#include "third_party/blink/renderer/platform/bindings/trace_wrapper_v8_reference.h"
//#include "third_party/blink/renderer/platform/bindings/wrapper_type_info.h"
//#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
//#include "third_party/blink/renderer/platform/platform_export.h"
//#include "third_party/blink/renderer/platform/wtf/type_traits.h"
#include <v8/v8.h>
#include "platform/heap/garbage_collected.h"

namespace webf {

class DOMDataStore;
class ScriptState;

// ScriptWrappable provides a way to map from/to C++ DOM implementation to/from
// JavaScript object (platform object).  ToV8() converts a ScriptWrappable to
// a v8::Object and toScriptWrappable() converts a v8::Object back to
// a ScriptWrappable.  v8::Object as platform object is called "wrapper object".
// The wrapper object for the main world is stored in ScriptWrappable.  Wrapper
// objects for other worlds are stored in DOMDataStore.
class ScriptWrappable
    : public GarbageCollected<ScriptWrappable>,
      public NameClient {
 public:
  // This is a type dispatcher from ScriptWrappable* to a subtype, optimized for
  // use cases that perform downcasts multiple times. If you perform a downcast
  // only once, ScriptWrappable::DowncastTo or ScriptWrappable::ToMostDerived
  // would be a better choice.
  class TypeDispatcher final {
    STACK_ALLOCATED();

   public:
    // The input parameter `script_wrappable` must not be null.
    explicit TypeDispatcher(ScriptWrappable* script_wrappable)
        : script_wrappable_(script_wrappable),
          wrapper_type_info_(script_wrappable->GetWrapperTypeInfo()) {}
    ~TypeDispatcher() = default;

    TypeDispatcher(const TypeDispatcher&) = delete;
    TypeDispatcher& operator=(const TypeDispatcher&) = delete;

    // Downcasts the ScriptWrappable to the given template parameter type or
    // nullptr if the ScriptWrappable doesn't implement the given type. The
    // inheritance is checked with WrapperTypeInfo, i.e. the check is based on
    // the IDL definitions in *.idl files, not based on C++ class inheritance.
    template <typename T>
    T* DowncastTo() {
      if (wrapper_type_info_->IsSubclass(T::GetStaticWrapperTypeInfo()))
        return static_cast<T*>(script_wrappable_);
      return nullptr;
    }

    // Downcasts the ScriptWrappable to the given template parameter type iff
    // the ScriptWrappable implements the type as the most derived class (i.e.
    // the ScriptWrappable does _not_ implement a subtype of the given type).
    // Otherwise, returns nullptr. The inheritance is checked with
    // WrapperTypeInfo, i.e. the check is based on the IDL definitions in *.idl
    // files, not based on C++ class inheritance.
    template <typename T>
    T* ToMostDerived() {
      if (wrapper_type_info_ == T::GetStaticWrapperTypeInfo())
        return static_cast<T*>(script_wrappable_);
      return nullptr;
    }

   private:
    ScriptWrappable* script_wrappable_ = nullptr;
    const WrapperTypeInfo* wrapper_type_info_ = nullptr;
  };

  ScriptWrappable(const ScriptWrappable&) = delete;
  ScriptWrappable& operator=(const ScriptWrappable&) = delete;
  ~ScriptWrappable() override = default;

  const char* NameInHeapSnapshot() const override;

  virtual void Trace(Visitor*) const;

  // Downcasts this instance to the given template parameter type or nullptr if
  // this instance doesn't implement the given type. The inheritance is checked
  // with WrapperTypeInfo, i.e. the check is based on the IDL definitions in
  // *.idl files, not based on C++ class inheritance.
  template <typename T>
  T* DowncastTo() {
    if (GetWrapperTypeInfo()->IsSubclass(T::GetStaticWrapperTypeInfo()))
      return static_cast<T*>(this);
    return nullptr;
  }

  // Downcasts this instance to the given template parameter type iff this
  // instance implements the type as the most derived class (i.e. this instance
  // does _not_ implement a subtype of the given type). Otherwise, returns
  // nullptr. The inheritance is checked with WrapperTypeInfo, i.e. the check is
  // based on the IDL definitions in *.idl files, not based on C++ class
  // inheritance.
  template <typename T>
  T* ToMostDerived() {
    if (GetWrapperTypeInfo() == T::GetStaticWrapperTypeInfo())
      return static_cast<T*>(this);
    return nullptr;
  }

  template <typename T>
  T* ToImpl() {  // DEPRECATED
    // All ScriptWrappables are managed by the Blink GC heap; check that
    // |T| is a garbage collected type.
    static_assert(
        sizeof(T) && WTF::IsGarbageCollectedType<T>::value,
        "Classes implementing ScriptWrappable must be garbage collected.");

    // Check if T* is castable to ScriptWrappable*, which means T doesn't
    // have two or more ScriptWrappable as superclasses. If T has two
    // ScriptWrappable as superclasses, conversions from T* to
    // ScriptWrappable* are ambiguous.
    static_assert(!static_cast<ScriptWrappable*>(static_cast<T*>(nullptr)),
                  "Class T must not have two or more ScriptWrappable as its "
                  "superclasses.");

    return static_cast<T*>(this);
  }

  // Returns the WrapperTypeInfo of the instance.
  //
  // This method must be overridden by DEFINE_WRAPPERTYPEINFO macro.
  virtual const WrapperTypeInfo* GetWrapperTypeInfo() const = 0;

  // Returns a wrapper object, creating it if needed.
  v8::Local<v8::Value> ToV8(ScriptState*);
  v8::Local<v8::Value> ToV8(v8::Isolate*,
                            v8::Local<v8::Object> creation_context_object);

  // Creates and returns a new wrapper object. This DCHECKs that a wrapper does
  // not exist yet. Use ToV8() if a wrapper might already exist.
  virtual v8::Local<v8::Value> Wrap(ScriptState*);

  // Associates the instance with the given |wrapper| if this instance is not
  // yet associated with any wrapper.  Returns the wrapper already associated
  // or |wrapper| if not yet associated.
  // The caller should always use the returned value rather than |wrapper|.
  [[nodiscard]] virtual v8::Local<v8::Object> AssociateWithWrapper(
      v8::Isolate*,
      const WrapperTypeInfo*,
      v8::Local<v8::Object> wrapper);

 protected:
  ScriptWrappable() = default;

 private:
  static_assert(
      std::is_trivially_destructible<
          TraceWrapperV8Reference<v8::Object>>::value,
      "TraceWrapperV8Reference<v8::Object> should be trivially destructible.");

  // Inline storage for the a single wrapper reference. Only
  // `DOMDataStore::UncheckedInlineStorageForWrappable()` should access this
  // field.
  TraceWrapperV8Reference<v8::Object> wrapper_;
  friend class DOMDataStore;
};

// Defines |GetWrapperTypeInfo| virtual method which returns the WrapperTypeInfo
// of the instance. Also declares a static member of type WrapperTypeInfo, of
// which the definition is given by the IDL code generator.
//
// All the derived classes of ScriptWrappable, regardless of directly or
// indirectly, must write this macro in the class definition as long as the
// class has a corresponding .idl file.
#define DEFINE_WRAPPERTYPEINFO()                               \
 public:                                                       \
  const WrapperTypeInfo* GetWrapperTypeInfo() const override { \
    return &wrapper_type_info_;                                \
  }                                                            \
  static const WrapperTypeInfo* GetStaticWrapperTypeInfo() {   \
    return &wrapper_type_info_;                                \
  }                                                            \
                                                               \
 private:                                                      \
  static const WrapperTypeInfo& wrapper_type_info_

}  // namespace webf

#endif  // WEBF_SCRIPT_WRAPPABLE_H
