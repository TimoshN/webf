/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/scheduler.dart';
import 'package:webf/dom.dart';
import 'package:webf/webf.dart';
import 'package:webf/css.dart';
import 'package:path/path.dart';

// Children of the <head> element all have display:none
const Map<String, dynamic> _defaultStyle = {
  DISPLAY: NONE,
};

const String _MIME_TEXT_JAVASCRIPT = 'text/javascript';
const String _MIME_APPLICATION_JAVASCRIPT = 'application/javascript';
const String _MIME_X_APPLICATION_JAVASCRIPT = 'application/x-javascript';
const String _MIME_X_APPLICATION_KBC = 'application/vnd.webf.bc1';
const String _JAVASCRIPT_MODULE = 'module';
enum ScriptReadyState { loading, interactive, complete }

typedef ScriptExecution = Future<void> Function(bool async);

class ScriptRunner {
  ScriptRunner(Document document, double contextId)
      : _document = document,
        _contextId = contextId;
  final Document _document;
  final double _contextId;

  final List<ScriptExecution> _syncScriptTasks = [];
  final List<ScriptExecution> _preloadScriptTasks = [];

  // Indicate the sync pending scripts.
  int _resolvingCount = 0;

  static Future<void> _evaluateScriptBundle(double contextId, WebFBundle bundle, {bool async = false}) async {
    // Evaluate bundle.
    if (bundle.isJavascript) {
      assert(isValidUTF8String(bundle.data!), 'The JavaScript codes should be in UTF-8 encoding format');
      bool result = await evaluateScripts(contextId, bundle.data!, url: bundle.url);
      if (!result) {
        throw FlutterError('Script code are not valid to evaluate.');
      }
    } else if (bundle.isBytecode) {
      bool result = await evaluateQuickjsByteCode(contextId, bundle.data!);
      if (!result) {
        throw FlutterError('Bytecode are not valid to execute.');
      }
    } else {
      throw FlutterError('Unknown type for <script> to execute. $url');
    }
  }

  void _execute(List<ScriptExecution> tasks, {bool async = false}) async {
    List<ScriptExecution> executingTasks = [...tasks];
    tasks.clear();

    for (ScriptExecution task in executingTasks) {
      await task(async);
    }
  }

  bool hasPreloadScripts() {
    return _preloadScriptTasks.isNotEmpty;
  }

  bool hasPendingScripts() {
    return _syncScriptTasks.isNotEmpty;
  }

  void _queueScriptForExecution(ScriptElement element, {bool isInline = false}) async {
    // Increment load event delay count before eval.
    _document.incrementDOMContentLoadedEventDelayCount();

    // Increase the pending count for preloading resources.
    if (_document.controller.preloadStatus != PreloadingStatus.none) {
      _document.controller.unfinishedPreloadResources++;
    }

    // Obtain bundle.
    WebFBundle bundle;
    bool isInPreLoading = _document.controller.mode == WebFLoadingMode.preloading &&
        _document.controller.preloadStatus != PreloadingStatus.done;

    if (isInline) {
      String? scriptCode = element.collectElementChildText();
      if (scriptCode == null) {
        return;
      }
      bundle = WebFBundle.fromContent(scriptCode);
    } else {
      String url = element.src.toString();
      bundle = _document.controller.getPreloadBundleFromUrl(url) ?? WebFBundle.fromUrl(url);
    }

    element.readyState = ScriptReadyState.interactive;
    // The bundle execution task.
    Future<void> task(bool async) async {
      // If bundle is not resolved, should wait for it resolve to prevent the next script running.
      assert(bundle.isResolved, '${bundle.url} is not resolved');

      try {
        await _evaluateScriptBundle(_contextId, bundle, async: async);
      } catch (err, stack) {
        debugPrint('$err\n$stack');
        _document.decrementDOMContentLoadedEventDelayCount();
        await bundle.invalidateCache();
        return;
      } finally {
        bundle.dispose();
      }

      element.readyState = ScriptReadyState.complete;
      // Dispatch the load event.
      Timer.run(() {
        element.dispatchEvent(Event(EVENT_LOAD));
      });

      // Decrement load event delay count after eval.
      _document.decrementDOMContentLoadedEventDelayCount();
    }

    // @TODO: Differ async and defer.
    final bool shouldAsync = element.async || element.defer;
    if (!shouldAsync) {
      if (isInPreLoading) {
        _preloadScriptTasks.add(task);
      } else {
        _syncScriptTasks.add(task);
      }

      _resolvingCount++;
    }

    // Script loading phrase.
    // Increment count when request.
    _document.incrementDOMContentLoadedEventDelayCount();
    try {
      await bundle.resolve(baseUrl: _document.controller.url, uriParser: _document.controller.uriParser);
      await bundle.obtainData();

      if (!bundle.isResolved) {
        throw FlutterError('Network error.');
      }
    } catch (e, st) {
      // A load error occurred.
      debugPrint('Failed to load: $url, reason: $e\n$st');
      Timer.run(() {
        element.dispatchEvent(Event(EVENT_ERROR));
      });
      _document.decrementDOMContentLoadedEventDelayCount();
      // Cancel failed task.
      if (!isInPreLoading) {
        _syncScriptTasks.remove(task);
      }
      return;
    } finally {
      // Decrease the resolving count.
      if (!shouldAsync) {
        _resolvingCount--;
      }

      // Decrement count when response.
      _document.decrementDOMContentLoadedEventDelayCount();
    }

    if (!isInPreLoading) {
      // Script executing phrase.
      if (shouldAsync) {
        SchedulerBinding.instance.scheduleFrameCallback((_) async {
          await task(shouldAsync);
        });
      } else {
        scheduleMicrotask(() {
          if (_resolvingCount == 0) {
            _execute(_syncScriptTasks, async: false);
          }
        });
      }
    } else {
      await bundle.preProcessing(_contextId);
      _document.pendingPreloadingScriptCallbacks.add(() async => await task(shouldAsync));

      if (_document.controller.preloadStatus != PreloadingStatus.none) {
        _document.controller.unfinishedPreloadResources--;
        _document.controller.checkPreloadCompleted();
      }
    }
  }

  Future<void> executePreloadedBundles() async {
    _execute(_preloadScriptTasks);
  }
}

// https://www.w3.org/TR/2011/WD-html5-author-20110809/the-link-element.html
class ScriptElement extends Element {
  ScriptElement([BindingContext? context]) : super(context);

  @override
  Map<String, dynamic> get defaultStyle => _defaultStyle;

  Uri? _resolvedSource;
  ScriptReadyState _readyState = ScriptReadyState.loading;

  @override
  void initializeProperties(Map<String, BindingObjectProperty> properties) {
    super.initializeProperties(properties);

    properties['src'] = BindingObjectProperty(getter: () => src, setter: (value) => src = castToType<String>(value));
    properties['async'] =
        BindingObjectProperty(getter: () => async, setter: (value) => async = castToType<bool>(value));
    properties['defer'] =
        BindingObjectProperty(getter: () => defer, setter: (value) => defer = castToType<bool>(value));
    properties['charset'] =
        BindingObjectProperty(getter: () => charset, setter: (value) => charset = castToType<String>(value));
    properties['type'] = BindingObjectProperty(getter: () => type, setter: (value) => type = castToType<String>(value));
    properties['text'] = BindingObjectProperty(getter: () => text, setter: (value) => text = castToType<String>(value));
    properties['readyState'] = BindingObjectProperty(
      getter: () => readyState.name,
    );
  }

  String get src => _resolvedSource?.toString() ?? '';

  set src(String value) {
    _resolveSource(value);
    _fetchAndExecuteSource();
    // Set src will not reflect to attribute src.
  }

  bool _async = false;
  bool get async => _async;

  set async(bool value) {
    _async = value;
  }

  bool _defer = false;
  bool get defer => _defer;
  set defer(bool value) {
    _defer = value;
  }

  String _type = _MIME_TEXT_JAVASCRIPT;
  String get type => _type;
  set type(String value) {
    _type = value;
  }

  String _charset = '';
  String get charset => _charset;
  set charset(String value) {
    _charset = value;
  }

  String _text = '';
  String get text => _text;
  set text(String value) {
    _text = value;
  }

  ScriptReadyState get readyState => _readyState;

  set readyState(ScriptReadyState readyState) {
    _readyState = readyState;
  }

  void _resolveSource(String source) {
    String base = ownerDocument.controller.url;
    try {
      _resolvedSource = ownerDocument.controller.uriParser!.resolve(Uri.parse(base), Uri.parse(source));
    } catch (_) {
      // Ignoring the failure of resolving, but to remove the resolved hyperlink.
      _resolvedSource = null;
    }
  }

  void _fetchAndExecuteSource() async {
    double? contextId = ownerDocument.contextId;
    if (contextId == null) return;
    // Must
    if (src.isNotEmpty &&
        isConnected &&
        (_type == _MIME_TEXT_JAVASCRIPT ||
            _type == _MIME_APPLICATION_JAVASCRIPT ||
            _type == _MIME_X_APPLICATION_JAVASCRIPT ||
            _type == _MIME_X_APPLICATION_KBC ||
            _type == _JAVASCRIPT_MODULE)) {
      // Add bundle to scripts queue.
      ownerDocument.scriptRunner._queueScriptForExecution(this);

      SchedulerBinding.instance.scheduleFrame();
    } else if (childNodes.isNotEmpty) {
      ownerDocument.scriptRunner._queueScriptForExecution(this, isInline: true);
    }
  }

  @override
  void connectedCallback() async {
    super.connectedCallback();
    double? contextId = ownerDocument.contextId;
    if (contextId == null) return;
    if (src.isNotEmpty) {
      _fetchAndExecuteSource();
    } else if (_type == _MIME_TEXT_JAVASCRIPT || _type == _JAVASCRIPT_MODULE) {
      _fetchAndExecuteSource();
    }
  }
}
