/**
 * SPDX-FileCopyrightText: Copyright 2023 Open Mobile Platform LLC <community@omp.ru>
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef FLUTTER_PLUGIN_WEBF_PLUGIN_H
#define FLUTTER_PLUGIN_WEBF_PLUGIN_H

#include <webf/encodable_helper.h>


#include <flutter/flutter_aurora.h>
#include <flutter/plugin_registrar.h>
#include <flutter/method_channel.h>
#include <flutter/event_channel.h>
#include <flutter/encodable_value.h>
#include <flutter/standard_message_codec.h>
#include <flutter/standard_method_codec.h>
#include <flutter/event_stream_handler_functions.h>


typedef flutter::Plugin Plugin;
typedef flutter::PluginRegistrar PluginRegistrar;
typedef flutter::MethodChannel<EncodableValue> MethodChannel;
typedef flutter::MethodCall<EncodableValue> MethodCall;
typedef flutter::MethodResult<EncodableValue> MethodResult;
typedef flutter::EventChannel<EncodableValue> EventChannel;
typedef flutter::EventSink<EncodableValue> EventSink;

#ifdef PLUGIN_IMPL
#define PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#define PLUGIN_EXPORT
#endif

class PLUGIN_EXPORT WebfPlugin final : public flutter::Plugin
{
public:
    static void RegisterWithRegistrar(PluginRegistrar* registrar);
    WebfPlugin(
    PluginRegistrar* registrar,
    std::unique_ptr<MethodChannel> methodChannel
);
private:

    void RegisterMethodHandler();
    void RegisterStreamHandler();

//    void onMethodCall(const MethodCall &call);
    std::string onGetPlatformVersion(const MethodCall &call);
    std::string onGetTemporaryDirectory(const MethodCall &call);
    void unimplemented(const MethodCall &call);

        std::unique_ptr<MethodChannel> m_methodChannel;


};

#endif /* FLUTTER_PLUGIN_WEBF_PLUGIN_H */
