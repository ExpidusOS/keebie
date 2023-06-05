import 'dart:convert';
import 'package:libtokyo_flutter/libtokyo.dart';

enum KeyboardKeyType {
  regular,
  shift,
  enter,
  backspace,
  space,
  plane,
}

class KeyboardKey {
  const KeyboardKey({
    required this.type,
    required this.name,
    required this.icon,
    required this.expands,
    this.plane,
  });

  final KeyboardKeyType type;
  final String name;
  final IconData? icon;
  final bool expands;
  final int? plane;
}

List<List<List<KeyboardKey>>> decodeKeyboard(String source) => (json.decode(source) as List<dynamic>).map((plane) => (plane as List<dynamic>).map((row) => (row as List<dynamic>).map((keyDynamic) {
  final data = keyDynamic as Map<String, dynamic>;
  return KeyboardKey(
    type: data.containsKey('type') ? KeyboardKeyType.values.firstWhere((e) => e.name == data['type']) : KeyboardKeyType.regular,
    name: data['name'] ?? '',
    icon: data.containsKey('icon') ?
      IconData(
        data['icon'],
        fontFamily: data['iconFontFamily']
      )
    : null,
    expands: data['expands'] ?? false,
    plane: data['plane'],
  );
}).toList()).toList()).toList();