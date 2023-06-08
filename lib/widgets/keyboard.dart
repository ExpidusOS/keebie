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
    this.isShifted = false
  });

  Keyboard.json({
    super.key,
    required String json,
    this.plane = 0,
    this.isShifted = false
  }) : layout = KeyboardLayout.fromJson(json), onLayout = null;

  Keyboard.asset({
    super.key,
    required String name,
    this.plane = 0,
    this.isShifted = false
  })
    : layout = null,
      onLayout = onLayoutAsset(name);

  final int plane;
  final bool isShifted;
  final KeyboardLayout? layout;
  final Future<KeyboardLayout> Function()? onLayout;

  @override
  State<Keyboard> createState() => _KeyboardState();
}

class _KeyboardState extends State<Keyboard> {
  late int plane;
  late bool isShifted;
  bool isAnnounced = false;
  List<KeyboardKeyConstraint> constraints = <KeyboardKeyConstraint>[];

  @override
  void initState() {
    super.initState();

    plane = widget.plane;
    isShifted = widget.isShifted;

    Keebie.constraints.then((value) => setState(() {
      constraints = value;
    })).catchError((error, trace) {
      handleError(error, trace: trace);
    });
  }

  Widget buildKey(BuildContext context, KeyboardLayout layout, KeyboardKey key, int rowNo, int keyNo) {
    var textColor = Colors.white;
    var backgroundColor = ButtonTheme.of(context).colorScheme!.onSurface;

    if (key.secondaryColors || (key.type == KeyboardKeyType.shift && isShifted)) {
      textColor = Colors.black;
      backgroundColor = ButtonTheme.of(context).colorScheme!.onSecondary;
    }

    Widget? child;
    if (key.name.isNotEmpty) {
      child = Text(
        key.name,
        style: Theme.of(context).textTheme.labelSmall!.copyWith(color: textColor),
      );
    } else if (key.icon != null) {
      child = Icon(
        key.icon!,
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

    Widget widget = Padding(
      padding: const EdgeInsets.all(2.0),
      child: InkWell(
        child: SizedBox(
          height: (MediaQuery.sizeOf(context).height / layout.planes[plane].length) / 1.5,
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

    return key.expands ? SizedBox(
      width: MediaQuery.sizeOf(context).width / 2,
      child: widget,
    ) : Expanded(child: widget);
  }

  Widget buildLayout(BuildContext context, KeyboardLayout layout) {
    if (!isAnnounced) {
      Keebie.announceLayout(layout).then((nothing) {
        isAnnounced = true;
      }).catchError((error, trace) {
        handleError(error, trace: trace);
      });
    }

    return Column(
      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
      children: layout.planes[plane].asMap().map((rowNo, keys) =>
        MapEntry(rowNo, Row(
          mainAxisAlignment: MainAxisAlignment.spaceAround,
          children: (keys..retainWhere((key) {
            return !key.constrains.map(constraints.contains).contains(false);
          })).asMap().map((keyNo, key) => MapEntry(keyNo, buildKey(context, layout, key, rowNo, keyNo))).values.toList(),
        ))
      ).values.toList()
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