/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

import 'package:meta/meta.dart';
import 'package:flutter/widgets.dart';
import 'package:webf/foundation.dart';
import 'package:webf/css.dart';
import 'package:webf/dom.dart' as dom;
import 'webf_adapter_widget.dart';
import 'render_object_to_widget_adaptor.dart';

const Map<String, dynamic> _defaultStyle = {
  DISPLAY: BLOCK,
};

// WidgetElement is the base class for custom elements which rendering details are implemented by Flutter widgets.
abstract class WidgetElement extends dom.Element {
  // An state
  late WebFWidgetElementWidget _widget;
  WebFWidgetElementWidget get widget => _widget;

  WebFWidgetElementState? _state;
  set state(WebFWidgetElementState? newState) {
    _state = newState;
  }
  WebFWidgetElementToWidgetAdapter? attachedAdapter;

  WidgetElement(
    BindingContext? context, {
    Map<String, dynamic>? defaultStyle,
  }) : super(
          context,
          defaultStyle: {
            ..._defaultStyle,
            ...?defaultStyle
          }) {
    WidgetsFlutterBinding.ensureInitialized();
    _widget = WebFWidgetElementWidget(this);
  }

  // State methods, proxy called from _state
  void initState() {}

  // React to properties and attributes changes
  void attributeDidUpdate(String key, String value) {}
  bool shouldElementRebuild(String key, previousValue, nextValue) {
    return previousValue == nextValue;
  }
  void propertyDidUpdate(String key, value) {}
  void styleDidUpdate(String property, String value) {}

  Widget build(BuildContext context, List<Widget> children);

  // The render object is inserted by Flutter framework when element is WidgetElement.
  @override
  dom.RenderObjectManagerType get renderObjectManagerType => dom.RenderObjectManagerType.FLUTTER_ELEMENT;

  @nonVirtual
  @override
  bool get isWidgetElement => true;

  @mustCallSuper
  @override
  void didDetachRenderer() {
    super.didDetachRenderer();
    _detachWidget();
  }

  @nonVirtual
  void setState(VoidCallback callback) {
    if (_state != null) {
      _state!.requestUpdateState(callback);
    }
  }

  /// [willAttachRenderer] and [didAttachRenderer] on WidgetElement will be called when this WidgetElement's parent is
  /// standard WebF element.
  /// If this WidgetElement's parent is another WidgetElement, [WebFWidgetElementElement.mount] and [WebFWidgetElementElement.unmount]
  /// place the equivalence altitude to the [willAttachRenderer] and [didAttachRenderer].
  @mustCallSuper
  @override
  void willAttachRenderer() {
    super.willAttachRenderer();
    attachedAdapter = WebFWidgetElementToWidgetAdapter(child: widget, container: renderBoxModel!);
  }
  @mustCallSuper
  @override
  void didAttachRenderer() {
    // Children of WidgetElement should insert render object by Flutter Framework.
    _attachWidget(_widget);
  }

  @override
  void setInlineStyle(String property, String value) {
    super.setInlineStyle(property, value);
    bool shouldRebuild = shouldElementRebuild(property, style.getPropertyValue(property), value);
    if (_state != null && shouldRebuild) {
      _state!.requestUpdateState();
    }
    styleDidUpdate(property, value);
  }

  @mustCallSuper
  @override
  void removeAttribute(String key) {
    super.removeAttribute(key);
    bool shouldRebuild = shouldElementRebuild(key, getAttribute(key), null);
    if (_state != null && shouldRebuild) {
      _state!.requestUpdateState();
    }
    attributeDidUpdate(key, '');
  }

  @mustCallSuper
  @override
  void setAttribute(String key, value) {
    super.setAttribute(key, value);
    bool shouldRebuild = shouldElementRebuild(key, getAttribute(key), value);
    if (_state != null && shouldRebuild) {
      _state!.requestUpdateState();
    }
    attributeDidUpdate(key, value);
  }

  @nonVirtual
  @override
  dom.Node appendChild(dom.Node child) {
    super.appendChild(child);

    if (_state != null) {
      _state!.requestUpdateState();
    }

    return child;
  }

  @nonVirtual
  @override
  dom.Node insertBefore(dom.Node child, dom.Node referenceNode) {
    dom.Node inserted = super.insertBefore(child, referenceNode);

    if (_state != null) {
      _state!.requestUpdateState();
    }

    return inserted;
  }

  @nonVirtual
  @override
  dom.Node? replaceChild(dom.Node newNode, dom.Node oldNode) {
    dom.Node? replaced = super.replaceChild(newNode, oldNode);

    if (_state != null) {
      _state!.requestUpdateState();
    }

    return replaced;
  }

  @nonVirtual
  @override
  dom.Node removeChild(dom.Node child) {
    super.removeChild(child);

    if (_state != null) {
      _state!.requestUpdateState();
    }

    return child;
  }

  static dom.Node? _getAncestorWidgetNode(WidgetElement element) {
    dom.Node? parent = element.parentNode;

    while(parent != null) {
      if (parent.flutterWidget != null) {
        return parent;
      }

      parent = parent.parentNode;
    }

    return null;
  }

  void _attachWidget(Widget widget) {
    dom.Node? ancestorWidgetNode = _getAncestorWidgetNode(this);
    if (ancestorWidgetNode != null) {
      (ancestorWidgetNode as dom.Element).flutterWidgetState!.addWidgetChild(attachedAdapter!);
    } else {
      ownerDocument.controller.onCustomElementAttached!(attachedAdapter!);
    }
  }

  void _detachWidget() {
    if (attachedAdapter != null) {
      dom.Node? ancestorWidgetNode = _getAncestorWidgetNode(this);
      if (ancestorWidgetNode != null) {
        (ancestorWidgetNode as dom.Element).flutterWidgetState!.removeWidgetChild(attachedAdapter!);
      } else {
        ownerDocument.controller.onCustomElementDetached!(attachedAdapter!);
      }
      attachedAdapter = null;
    }
  }
}
