import 'package:bitsdojo_window/bitsdojo_window.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:libtokyo_flutter/libtokyo.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:keebie/logic.dart';

class KeyboardView extends StatefulWidget {
  const KeyboardView({ super.key, required this.name });

  final String name;

  @override
  State<KeyboardView> createState() => _KeyboardViewState();
}

class _KeyboardViewState extends State<KeyboardView> {
  int plane = 0;
  bool isShifted = false;
  final methodChannel = const MethodChannel("keebie");

  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) =>
      Scaffold(
        windowBar: WindowBar.shouldShow(context) && !kIsWeb ? PreferredSize(
          preferredSize: const Size.fromHeight(kToolbarHeight / 2),
          child: MoveWindow(
            child: WindowBar(
              leading: Image.asset('assets/imgs/icon.png'),
              title: Text(AppLocalizations.of(context)!.applicationTitle),
              onMinimize: () => appWindow.minimize(),
              onMaximize: () => appWindow.maximize(),
              onClose: () => appWindow.close(),
            ),
          ),
        ) : null,
        body: FutureBuilder(
          future: rootBundle.loadString('assets/keyboards/${widget.name}.json'),
          builder: (context, snapshot) {
            if (snapshot.hasError) {
              return BasicCard(
                color: convertFromColor(Theme.of(context).colorScheme.onError),
                title: AppLocalizations.of(context)!.genericErrorMessage(snapshot.error.toString()),
              );
            }

            if (snapshot.hasData) {
              final keyboardRows = decodeKeyboard(snapshot.data!);
              return Column(
                mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                children: [
                  ...(keyboardRows[plane].map((keys) => Row(
                    mainAxisAlignment: MainAxisAlignment.spaceAround,
                    children: keys.map((key) {
                      var textColor = Colors.white;
                      var backgroundColor = ButtonTheme.of(context).colorScheme!.onSurface;

                      if (key.secondaryColors || (key.type == KeyboardKeyType.shift && isShifted)) {
                        textColor = Colors.black;
                        backgroundColor = ButtonTheme.of(context).colorScheme!.onSecondary;
                      }

                      Widget? icon;
                      if (key.name.isNotEmpty) {
                        icon = Text(
                          key.name,
                          style: Theme.of(context).textTheme.labelSmall!.copyWith(color: textColor),
                        );
                      } else if (key.icon != null) {
                        icon = Icon(
                          key.icon!,
                          color: textColor
                        );
                      }

                      if (isShifted) {
                        if (key.shiftedName.isNotEmpty) {
                          icon = Text(
                            key.shiftedName,
                            style: Theme.of(context).textTheme.labelSmall!.copyWith(color: textColor),
                          );
                        } else if (key.shiftedIcon != null) {
                          icon = Icon(
                            key.shiftedIcon!,
                            color: textColor
                          );
                        }
                      }

                      Widget child = Padding(
                        padding: const EdgeInsets.all(0.5),
                        child: FloatingActionButton.small(
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(8.0),
                          ),
                          backgroundColor: backgroundColor,
                          materialTapTargetSize: MaterialTapTargetSize.shrinkWrap,
                          child: icon,
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
                        child = Expanded(child: child);
                      }
                      return child;
                    }).toList(),
                  )).toList())
                ]
              );
            }
            return const CircularProgressIndicator();
          },
        ),
      );
}