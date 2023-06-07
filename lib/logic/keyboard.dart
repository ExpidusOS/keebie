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

  dynamic toJson() {
    return {
      'type': type.name,
      'name': name,
      'shiftedName': shiftedName,
      'icon': icon == null ? null : icon!.codePoint,
      'iconFontFamily': icon == null ? null : icon!.fontFamily,
      'shiftedIcon': shiftedIcon == null ? null : shiftedIcon!.codePoint,
      'shiftedIconFontFamily': shiftedIcon == null ? null : shiftedIcon!.fontFamily,
      'expands': expands,
      'secondaryColors': secondaryColors,
      'plane': plane,
    };
  }
}

class KeyboardLayout {
  const KeyboardLayout({
    required this.locale,
    required this.planes,
  });

  final String locale;
  final List<List<List<KeyboardKey>>> planes;

  dynamic toJson() {
    return {
      'locale': locale,
      'planes': planes.map((plane) => plane.map((row) => row.map((key) => key.toJson()).toList()).toList()).toList()
    };
  }

  static KeyboardLayout fromJson(String source) {
    final data = json.decode(source);
    return KeyboardLayout(
      locale: data['locale'],
      planes: (data['planes'] as List<dynamic>).map((plane) => (plane as List<dynamic>).map((row) => (row as List<dynamic>).map((keyDynamic) {
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
      }).toList()).toList()).toList(),
    );
  }
}