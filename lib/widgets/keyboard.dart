import 'package:bitsdojo_window/bitsdojo_window.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart' hide KeyboardKey;
import 'package:libtokyo_flutter/libtokyo.dart';
import 'package:keebie/logic.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

Future<KeyboardLayout> Function() onLayoutAsset(String name) =>
    () async => KeyboardLayout.fromJson(await rootBundle.loadString('assets/keyboards/$name.json'));

class Keyboard extends StatefulWidget {
  const Keyboard({
    super.key,
    this.layout,
    this.onLayout,
    this.plane = 0,
    this.contentType,
    this.isShifted = false
  });

  Keyboard.json({
    super.key,
    required String json,
    this.plane = 0,
    this.contentType,
    this.isShifted = false
  }) : layout = KeyboardLayout.fromJson(json), onLayout = null;

  Keyboard.asset({
    super.key,
    required String name,
    this.plane = 0,
    this.contentType,
    this.isShifted = false
  })
    : layout = null,
      onLayout = onLayoutAsset(name);

  final int plane;
  final bool isShifted;
  final KeyboardContentType? contentType;
  final KeyboardLayout? layout;
  final Future<KeyboardLayout> Function()? onLayout;

  @override
  State<Keyboard> createState() => _KeyboardState();
}

class _KeyboardState extends State<Keyboard> {
  late int plane;
  late bool isShifted;
  bool isAnnounced = false;
  KeyboardContentType? contentType;
  List<KeyboardKeyConstraint> constraints = <KeyboardKeyConstraint>[];

  @override
  void initState() {
    super.initState();

    plane = widget.plane;
    isShifted = widget.isShifted;
    contentType = widget.contentType;

    Keebie.constraints.then((value) => setState(() {
      constraints = value;
    })).catchError((error, trace) {
      handleError(error, trace: trace);
    });

    if (contentType == null) {
      Keebie.contentType.then((value) => setState(() {
        contentType = value;
      })).catchError((error, trace) {
        handleError(error, trace: trace);
      });
    }
  }

  Widget buildKey(BuildContext context, KeyboardLayout layout, KeyboardKey key, int rowNo, int keyNo) {
    var textColor = Colors.white;
    var backgroundColor = ButtonTheme.of(context).colorScheme!.onSurface;

    if (key.secondaryColors || (key.type == KeyboardKeyType.shift && isShifted)) {
      textColor = Colors.black;
      backgroundColor = ButtonTheme.of(context).colorScheme!.onSecondary;
    }

    final textStyle = Theme.of(context).textTheme.labelSmall!.copyWith(
      color: textColor,
      fontSize: key.getChildSize(context).height,
    );

    Widget? child;
    if (key.name.isNotEmpty) {
      child = Text(
        key.name,
        style: textStyle,
      );
    } else if (key.icon != null) {
      child = Icon(
        key.icon!,
        size: textStyle.fontSize! * 2.0,
        color: textColor
      );
    }

    if (isShifted) {
      if (key.shiftedName.isNotEmpty) {
        child = Text(
          key.shiftedName,
          style: Theme.of(context).textTheme.labelSmall!.copyWith(color: textColor),
        );
      } else if (key.shiftedIcon != null) {
        child = Icon(
          key.shiftedIcon!,
          color: textColor
        );
      }
    }

    final currentPlane = layout.getPlane(
      plane,
      contentType: contentType
    );
    final row = currentPlane.rows[rowNo];
    final size = key.getContainerSize(context: context, row: row);

    return Padding(
      padding: KeyboardKey.padding,
      child: InkWell(
        child: SizedBox(
          height: size.height,
          width: size.width,
          child: Material(
            shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(8.0),
            ),
            color: backgroundColor,
            child: Center(child: child),
          ),
        ),
        onTap: () {
          switch (key.type) {
            case KeyboardKeyType.plane:
              setState(() {
                plane = key.plane!;
                isShifted = false;
              });
              break;
            case KeyboardKeyType.shift:
              setState(() {
                isShifted = !isShifted;
              });
              break;
            default:
              setState(() {
                Keebie.sendKey(key,
                  isShifted: isShifted,
                  rowNo: rowNo,
                  keyNo: keyNo,
                ).catchError((error, trace) => handleError(error, trace: trace));

                if (isShifted) {
                  isShifted = false;
                }
              });
              break;
          }
        },
      ),
    );
  }

  Widget buildLayout(BuildContext context, KeyboardLayout layout) {
    if (!isAnnounced) {
      Keebie.announceLayout(layout).then((nothing) {
        isAnnounced = true;
      }).catchError((error, trace) {
        handleError(error, trace: trace);
      });
    }

    final currentPlane = layout.getPlane(
      plane,
      contentType: contentType
    );

    final size = currentPlane.getSize(context, constraints: constraints);
    final viewSize = MediaQuery.sizeOf(context);
    final optimalSizeRaw = (size - viewSize) as Offset;
    final optimalSize = Size(optimalSizeRaw.dx.abs(), optimalSizeRaw.dy.abs());

    Widget layoutWidget = Column(
      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
      children: currentPlane.rows.map((row) => Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: row.getKeys(constraints: constraints)
          .asMap()
          .map((keyNo, key) =>
            MapEntry(
              keyNo,
              buildKey(context, layout, key, row.number, keyNo)
            )
          ).values.toList()
      )).toList(),
    );

    switch (defaultTargetPlatform) {
      case TargetPlatform.windows:
      case TargetPlatform.macOS:
      case TargetPlatform.linux:
        if (optimalSize != appWindow.size && mounted) {
          appWindow.minSize = appWindow.size = Size(optimalSize.width / 3.1, optimalSize.height / 6);
          appWindow.show();
        }
        break;
      default:
        break;
    }

    return SizedBox(
      width: optimalSize.width,
      height: optimalSize.height,
      child: layoutWidget,
    );
  }

  @override
  Widget build(BuildContext context) {
    if (widget.onLayout != null) {
      return FutureBuilder(
        future: widget.onLayout!(),
        builder: (context, snapshot) {
          if (snapshot.hasError) {
            return BasicCard(
              color: convertFromColor(Theme.of(context).colorScheme.onError),
              title: AppLocalizations.of(context)!.genericErrorMessage(snapshot.error.toString()),
            );
          }

          if (snapshot.hasData) {
            return buildLayout(context, snapshot.data!);
          }
          return const CircularProgressIndicator();
        },
      );
    }

    assert(widget.layout != null);
    return buildLayout(context, widget.layout!);
  }
}
