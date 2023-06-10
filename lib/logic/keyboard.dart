import 'dart:convert';
import 'dart:math';
import 'package:keebie/main.dart';
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
  static const padding = EdgeInsets.all(1.0);

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

  Size getChildSize(BuildContext context, bool isShifted, Rect monitorGeometry) {
    final textStyle = Theme.of(context).textTheme.labelLarge!;

    final pixelRatio = monitorGeometry.size.aspectRatio > MediaQuery.devicePixelRatioOf(context) ? monitorGeometry.size.aspectRatio : MediaQuery.devicePixelRatioOf(context);
    final initialSize = KeebieApp.getInitialSize(context);
    final scale = (initialSize.aspectRatio / pixelRatio);
    final value = Size.square(textStyle.fontSize!);
    return scale < 1.0 ? (value / scale) * 0.85 : value * scale;
  }

  Size getContainerSize({
    required Rect monitorGeometry,
    required BuildContext context,
    required KeyboardRow row,
    required bool isShifted,
    List<KeyboardKeyConstraint> constraints = const [],
  }) {
    final keys = row.getKeys(constraints: constraints);
    final childSize = getChildSize(context, isShifted, monitorGeometry);

    var height = (childSize.height * 1.5) + (KeyboardKey.padding.vertical * 2);
    var width = ((childSize.width * 1.5) + (KeyboardKey.padding.horizontal * 2));

    if (expands) {
      final usedSpace = keys.map((key) => key.expands ? Size.zero : key.getContainerSize(context: context, row: row, isShifted: isShifted, monitorGeometry: monitorGeometry))
        .reduce((v, e) => Size(v.width + e.width, e.height > v.height ? e.height : v.height));

      final largestRow = row.plane.findRowByLargest(context, constraints: constraints, isShifted: isShifted, monitorGeometry: monitorGeometry);
      final expandedWidth = largestRow.getSize(context, isShifted: isShifted, monitorGeometry: monitorGeometry).width - usedSpace.width;

      width = expandedWidth;
    }

    height = height.clamp(monitorGeometry.height / (height * row.plane.length), height);
    width = width.clamp(monitorGeometry.width / (width * keys.length), width);
    return Size(width, height);
  }

  Size getTotalSize({
    required Rect monitorGeometry,
    required BuildContext context,
    required KeyboardRow row,
    required bool isShifted,
    List<KeyboardKeyConstraint> constraints = const [],
  }) => getContainerSize(context: context, row: row, constraints: constraints, isShifted: isShifted, monitorGeometry: monitorGeometry)
    + Offset(padding.horizontal * 1.5, padding.vertical * 2);

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

class KeyboardRow {
  const KeyboardRow(this.plane, this._keys, this.number);

  final KeyboardPlane plane;
  final List<KeyboardKey> _keys;
  final int number;

  List<KeyboardKey> getKeys({
    List<KeyboardKeyConstraint> constraints = const [],
  }) {
    var entries = <KeyboardKey>[];

    for (final key in _keys) {
      if (!key.constrains.map(constraints.contains).contains(false)) {
        entries.add(key);
      }
    }
    return entries;
  }

  Size getSize(BuildContext context, {
    required bool isShifted,
    required Rect monitorGeometry,
    List<KeyboardKeyConstraint> constraints = const [],
  }) => getKeys(constraints: constraints)
    .map((key) => key.getTotalSize(context: context, row: this, isShifted: isShifted, monitorGeometry: monitorGeometry))
    .reduce((v, e) => Size(v.width + e.width, e.height > v.height ? e.height : v.height));
}

class KeyboardPlane {
  const KeyboardPlane(this._rows);

  final List<List<KeyboardKey>> _rows;

  int get length => _rows.length;

  List<KeyboardRow> get rows =>
    _rows.asMap().map((rowNo, row) =>
      MapEntry(rowNo, KeyboardRow(this, row, rowNo))
    ).values.toList();

  List<KeyboardRow> getFixedSizeRows({
    List<KeyboardKeyConstraint> constraints = const [],
  }) =>
    rows..removeWhere((row) {
      for (final key in row.getKeys(constraints: constraints)) {
        if (key.expands) return true;
      }
      return false;
    });

  KeyboardRow findRowByLargest(BuildContext context, {
    required bool isShifted,
    required Rect monitorGeometry,
    List<KeyboardKeyConstraint> constraints = const [],
    bool allowExpanded = false,
  }) {
    final r = allowExpanded ? rows : getFixedSizeRows(constraints: constraints);
    return rows[r.asMap().map((i, row) => MapEntry(row.number, row.getSize(context, constraints: constraints, isShifted: isShifted, monitorGeometry: monitorGeometry)))
        .entries.reduce((v, e) => e.value > v.value ? e : v).key];
  }

  Size getSize(BuildContext context, {
    required bool isShifted,
    required Rect monitorGeometry,
    List<KeyboardKeyConstraint> constraints = const [],
  }) => rows.map((row) => row.getSize(context, constraints: constraints, isShifted: isShifted, monitorGeometry: monitorGeometry))
    .reduce((v, e) => Size(e.width > v.width ? e.width : v.width, v.height + e.height));
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

  KeyboardPlane getPlane(int wanted, {
    KeyboardContentType? contentType,
  }) {
    if (contentType != null) {
      if (contentPlaneMap.containsKey(contentType)) {
        return getPlane(contentPlaneMap[contentType]!);
      }
      return const KeyboardPlane([]);
    }

    final rows = wanted < 0 || wanted > planes.length ? <List<KeyboardKey>>[] : planes[wanted];
    return KeyboardPlane(rows);
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