/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_TEST_WEBF_TEST_ENV_H_
#define BRIDGE_TEST_WEBF_TEST_ENV_H_

#include <memory>
#include "core/dart_methods.h"
#include "core/executing_context.h"
#include "core/page.h"
#include "foundation/logging.h"
//
//// Trigger a callbacks before GC free the eventTargets.
// using TEST_OnEventTargetDisposed = void (*)(binding::qjs::EventTargetInstance* eventTargetInstance);
// struct UnitTestEnv {
//  TEST_OnEventTargetDisposed onEventTargetDisposed{nullptr};
//};
//
//// Mock dart methods and add async timer to emulate webf environment in C++ unit test.
//

namespace webf {

std::unique_ptr<WebFPage> TEST_init(OnJSError onJsError);
std::unique_ptr<WebFPage> TEST_init();
std::unique_ptr<WebFPage> TEST_allocateNewPage();
void TEST_runLoop(ExecutingContext* context);
void TEST_mockDartMethods(int32_t contextId, OnJSError onJSError);

}  // namespace webf
// void TEST_dispatchEvent(int32_t contextId, EventTarget* eventTarget, const std::string type);
// void TEST_callNativeMethod(void* nativePtr, void* returnValue, void* method, int32_t argc, void* argv);
// void TEST_registerEventTargetDisposedCallback(int32_t contextUniqueId, TEST_OnEventTargetDisposed callback);
// std::shared_ptr<UnitTestEnv> TEST_getEnv(int32_t contextUniqueId);

#endif  // BRIDGE_TEST_WEBF_TEST_ENV_H_