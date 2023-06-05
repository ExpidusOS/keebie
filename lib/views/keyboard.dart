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
                children: keyboardRows[plane].map((keys) => Row(
                  mainAxisAlignment: MainAxisAlignment.spaceAround,
                  children: keys.map((key) {
                    Widget child = FloatingActionButton.small(
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(8.0),
                      ),
                      materialTapTargetSize: MaterialTapTargetSize.shrinkWrap,
                      child: key.name.isNotEmpty ? Text(key.name) : (key.icon != null ? Icon(key.icon!) : null),
                      onPressed: () {
                        switch (key.type) {
                          case KeyboardKeyType.plane:
                            setState(() {
                              plane = key.plane!;
                            });
                            break;
                          default:
                            break;
                        }
                      },
                    );

                    if (key.expands) {
                      child = Expanded(child: child);
                    }
                    return child;
                  }).toList(),
                )).toList(),
              );
            }
            return const CircularProgressIndicator();
          },
        ),
      );
}