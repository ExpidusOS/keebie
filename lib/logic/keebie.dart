import 'package:flutter/services.dart' hide KeyboardKey;
import 'package:keebie/logic.dart';

class Keebie {
  static const _methodChannel = MethodChannel('keebie');

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