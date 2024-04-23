/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef GIN_PUBLIC_WRAPPER_INFO_H_
#define GIN_PUBLIC_WRAPPER_INFO_H_

#include "bindings/v8/gin/gin_export.h"
#include <v8/v8-forward.h>
#include "bindings/v8/gin/public/gin_embedders.h"

namespace gin {

// Gin embedder that use their own WrapperInfo-like structs must ensure that
// the first field is of type GinEmbedderId and has the correct id set. They
// also should use kWrapperInfoIndex to start their WrapperInfo-like struct
// and ensure that all objects have kNumberOfInternalFields internal fields.

enum InternalFields {
  kWrapperInfoIndex,
  kEncodedValueIndex,
  kNumberOfInternalFields,
};

struct GIN_EXPORT WrapperInfo {
  static WrapperInfo* From(v8::Local<v8::Object> object);
  const GinEmbedder embedder;
};

}  // namespace gin

#endif  // GIN_PUBLIC_WRAPPER_INFO_H_