import 'package:flutter/services.dart' hide KeyboardKey;
import 'package:libtokyo_flutter/libtokyo.dart';
import 'package:keebie/logic.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

Future<KeyboardLayout> Function() onLayoutAsset(String name) =>
    () async => decodeKeyboard(await rootBundle.loadString('assets/keyboards/$name.json'));

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
  }) : layout = decodeKeyboard(json), onLayout = null;

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
  final methodChannel = const MethodChannel("keebie");

  @override
  void initState() {
    super.initState();

    plane = widget.plane;
    isShifted = widget.isShifted;
  }

  Widget buildKey(BuildContext context, KeyboardKey key) {
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
      padding: const EdgeInsets.all(0.5),
      child: FloatingActionButton.small(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(8.0),
        ),
        backgroundColor: backgroundColor,
        materialTapTargetSize: MaterialTapTargetSize.shrinkWrap,
        child: child,
        onPressed: () {
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
                methodChannel.invokeMethod('sendKey', {
                  'name': key.name,
                  'shiftedName': key.shiftedName,
                  'isShifted': isShifted,
                  'type': key.type.name,
                });

                if (isShifted) {
                  isShifted = false;
                }
              });
              break;
          }
        },
      ),
    );

    if (key.expands) {
      widget = Expanded(child: widget);
    }
    return widget;
  }

  Widget buildLayout(BuildContext context, KeyboardLayout layout) =>
    Column(
      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
      children: layout[plane].map((keys) => Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: keys.map((key) => buildKey(context, key)).toList(),
      )).toList()
    );

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