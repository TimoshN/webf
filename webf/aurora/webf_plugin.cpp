/**
 * SPDX-FileCopyrightText: Copyright 2023 Open Mobile Platform LLC <community@omp.ru>
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <webf/webf_plugin.h>


//#include <flutter/method-channel.h>
//#include <flutter/platform-methods.h>
#include <sys/utsname.h>

namespace PlatformVersion {

constexpr auto Version = "Linux";
constexpr auto Temp = "/tmp";

}


WebfPlugin::WebfPlugin(
    PluginRegistrar* registrar,
    std::unique_ptr<MethodChannel> methodChannel
) : m_methodChannel(std::move(methodChannel))
{
    RegisterMethodHandler();
}

void WebfPlugin::RegisterWithRegistrar(PluginRegistrar* registrar)
{
    auto methodChannel = std::make_unique<MethodChannel>(
        registrar->messenger(), "webf",
        &flutter::StandardMethodCodec::GetInstance());


    std::unique_ptr<WebfPlugin> plugin(new WebfPlugin(
        registrar,
        std::move(methodChannel)
    ));

    // Register plugin
    registrar->AddPlugin(std::move(plugin));

}

void WebfPlugin::RegisterMethodHandler()
{


    m_methodChannel->SetMethodCallHandler(
        [this](const MethodCall& call, std::unique_ptr<MethodResult> result) {
            if (call.method_name().compare("getPlatformVersion") == 0) {
                result->Success(onGetPlatformVersion(call));
            } else if (call.method_name().compare("getTemporaryDirectory") == 0) {
                result->Success(onGetTemporaryDirectory(call));
            } else {
                result->Success();
            }
        });


}

std::string WebfPlugin::onGetPlatformVersion(const MethodCall &call)
{
    return PlatformVersion::Version;
}

std::string WebfPlugin::onGetTemporaryDirectory(const MethodCall &call)
{
    return PlatformVersion::Temp;
}

void WebfPlugin::unimplemented(const MethodCall &call)
{
    //call.SendSuccessResponse(nullptr);
}
