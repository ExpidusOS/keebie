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
    required this.shiftedName,
    required this.icon,
    required this.shiftedIcon,
    required this.secondaryColors,
    required this.expands,
    this.plane,
  });

  final KeyboardKeyType type;
  final String name;
  final String shiftedName;
  final IconData? icon;
  final IconData? shiftedIcon;
  final bool expands;
  final bool secondaryColors;
  final int? plane;

  dynamic toJson() => {
    "type": type.name,
    "name": name,
  };
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
    shiftedName: data['shiftedName'] ?? '',
    shiftedIcon: data.containsKey('shiftedIcon') ?
      IconData(
        data['shiftedIcon'],
        fontFamily: data['shiftedIconFontFamily'] ?? data['iconFontFamily']
      )
    : null,
    expands: data['expands'] ?? false,
    secondaryColors: data['secondaryColors'] ?? false,
    plane: data['plane'],
  );
}).toList()).toList()).toList();