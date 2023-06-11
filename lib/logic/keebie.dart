import 'package:bitsdojo_window/bitsdojo_window.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart' hide KeyboardKey;
import 'package:keebie/logic.dart';

class Keebie {
  static const _methodChannel = MethodChannel('keebie');

  static void init() {
    _methodChannel.setMethodCallHandler((call) async {
      switch (call.method) {
        case 'onSettingsChange':
          if (onSettingsChange != null) {
            return await onSettingsChange!();
          }
          break;
        default:
          return null;
      }
    });
  }

  static Future<void> announceSettingsChange() =>
    _methodChannel.invokeMethod('announceSettingsChange');

  static Future<dynamic> Function()? onSettingsChange;

  static Future<void> openWindow({ bool keyboard = false }) =>
    _methodChannel.invokeMethod('openWindow', {
      'keyboard': keyboard
    });

  static Future<void> sendKey(KeyboardKey key, {
    required bool isShifted,
    required int rowNo,
    required int keyNo,
  }) => _methodChannel.invokeMethod('sendKey', {
    'name': key.name,
    'shiftedName': key.shiftedName,
    'type': key.type.name,
    'isShifted': isShifted,
    'rowNo': rowNo,
    'keyNo': keyNo,
  });

  static Future<void> announceLayout(KeyboardLayout layout) =>
    _methodChannel.invokeMethod('announceLayout', layout.toJson());

  static Future<List<KeyboardKeyConstraint>> get constraints async {
    try {
      return (await _methodChannel.invokeMethod('getConstraints') as List<dynamic>)
        .map((value) => KeyboardKeyConstraint.values.firstWhere((e) => e.name == value))
        .toList();
    } catch (e) {
      return [];
    }
  }

  static Future<KeyboardContentType?> get contentType async {
    try {
      final value = await _methodChannel.invokeMethod('getContentType');
      return KeyboardContentType.values.firstWhere((e) => e.name == value);
    } catch (e) {
      return null;
    }
  }

  static Future<bool> get isKeyboard async {
    try {
      return await _methodChannel.invokeMethod('isKeyboard');
    } catch (e) {
      return false;
    }
  }

  static Future<Rect> get monitorGeometry async {
    final value = await _methodChannel.invokeMethod('getMonitorGeometry');
    return Offset(value['x']!.toDouble(), value['y']!.toDouble())
      & Size(value['width']!.toDouble(), value['height']!.toDouble());
  }

  static set windowSize(Future<Size> size) {
    size.then((value) async {
      switch (defaultTargetPlatform) {
        case TargetPlatform.windows:
        case TargetPlatform.macOS:
        case TargetPlatform.linux:
          appWindow.minSize = value;
          appWindow.size = value;
          appWindow.show();
          break;
        default:
          await _methodChannel.invokeMethod('setWindowSize', {
            'width': value.width.round(),
            'height': value.height.round(),
          });
          break;
      }
    }).catchError((error, trace) {
      handleError(error, trace: trace);
    });
  }

  static Future<Size> get windowSize async {
    try {
      if (await isKeyboard) {
        final geom = await monitorGeometry;
        return Size(geom.width, geom.height / 3.15);
      }
    } finally {}
    return const Size(600, 450);
  }
}