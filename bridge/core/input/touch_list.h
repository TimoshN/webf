/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_CORE_INPUT_TOUCH_LIST_H_
#define BRIDGE_CORE_INPUT_TOUCH_LIST_H_

#include "touch.h"

namespace webf {

struct NativeTouchList {
  int64_t length;
  NativeTouch* touches;
};

class TouchList : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using ImplType = TouchList*;

  TouchList() = delete;
  explicit TouchList(ExecutingContext* context, NativeTouchList* native_touch_list);

  uint32_t length() const;
  Touch* item(uint32_t index, ExceptionState& exception_state) const;
  bool SetItem(uint32_t index, Touch* touch, ExceptionState& exception_state);

  bool NamedPropertyQuery(const AtomicString& key, ExceptionState& exception_state);
  void NamedPropertyEnumerator(std::vector<AtomicString>& props, ExceptionState& exception_state);

  void Trace(GCVisitor* visitor) const override;

 private:
  std::vector<Touch*> values_;
  NativeTouchList* native_touch_list_;
};

}  // namespace webf

#endif  // BRIDGE_CORE_INPUT_TOUCH_LIST_H_