import 'dart:convert';
import 'package:libtokyo_flutter/libtokyo.dart';

enum KeyboardKeyType {
  regular,
  shift,
  enter,
  backspace,
  space,
  plane,
  changeLang
}

enum KeyboardKeyConstraint {
  canChangeLanguage
}

enum KeyboardContentType {
  dateTime,
  number,
  phone,
  text
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
    required this.constrains,
    this.plane,
  });

  final KeyboardKeyType type;
  final String name;
  final String shiftedName;
  final IconData? icon;
  final IconData? shiftedIcon;
  final bool expands;
  final bool secondaryColors;
  final List<KeyboardKeyConstraint> constrains;
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
    required this.contentPlaneMap,
    required this.planes,
  });

  final String locale;
  final Map<KeyboardContentType, int> contentPlaneMap;
  final List<List<List<KeyboardKey>>> planes;

  List<List<KeyboardKey>> getPlane(int wanted, {
    KeyboardContentType? contentType,
  }) {
    if (contentType != null) {
      if (contentPlaneMap.containsKey(contentType)) {
        return getPlane(contentPlaneMap[contentType]!);
      }
      return [];
    }

    if (wanted < 0 || wanted > planes.length) return [];
    return planes[wanted];
  }

  dynamic toJson() {
    return {
      'locale': locale,
      'contentPlaneMap': contentPlaneMap.map((key, value) => MapEntry(key.name, value)),
      'planes': planes.map((plane) => plane.map((row) => row.map((key) => key.toJson()).toList()).toList()).toList(),
    };
  }

  static KeyboardLayout fromJson(String source) {
    final data = json.decode(source) as Map<String, dynamic>;
    return KeyboardLayout(
      locale: data['locale'],
      contentPlaneMap: data.containsKey('contentPlaneMap') ? (data['contentPlaneMap'] as Map<String, dynamic>).map((key, value) =>
        MapEntry(
          KeyboardContentType.values.firstWhere((e) => e.name == key),
          value
        )) : {},
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
          constrains: data.containsKey('constraints') ?
            (data['constraints'] as List<dynamic>)
              .map((value) => KeyboardKeyConstraint.values.firstWhere((e) => e.name == value)).toList()
          : [],
          plane: data['plane'],
        );
      }).toList()).toList()).toList(),
    );
  }
}