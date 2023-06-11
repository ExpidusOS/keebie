import 'package:bitsdojo_window/bitsdojo_window.dart';
import 'package:flutter/foundation.dart';
import 'package:keebie/logic.dart';
import 'package:keebie/widgets.dart';
import 'package:libtokyo_flutter/libtokyo.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

class KeyboardView extends StatefulWidget {
  const KeyboardView({ super.key, required this.name });

  final String name;

  @override
  State<KeyboardView> createState() => _KeyboardViewState();
}

class _KeyboardViewState extends State<KeyboardView> {
  @override
  Widget build(BuildContext context) =>
      Scaffold(
        windowBar: !kIsWeb && defaultTargetPlatform != TargetPlatform.linux && defaultTargetPlatform != TargetPlatform.android ? WindowBar(
          leading: Image.asset('assets/imgs/icon.png'),
          title: Text(AppLocalizations.of(context)!.applicationTitle),
        ) : null,
        appBar: PreferredSize(
          preferredSize: Size(
            MediaQuery.sizeOf(context).width,
            AppBar.preferredHeightFor(context, const Size.fromHeight(kToolbarHeight))
          ),
          child: Material(
            type: MaterialType.canvas,
            shadowColor: AppBarTheme.of(context).shadowColor,
            surfaceTintColor: AppBarTheme.of(context).surfaceTintColor,
            color: AppBarTheme.of(context).backgroundColor,
            child: Row(
              children: [
                IconButton(
                  icon: const Icon(Icons.settings),
                  onPressed: () => Keebie.openWindow(keyboard: false)
                    .catchError((error, trace) => handleError(error, trace: trace)),
                )
              ],
            ),
          ),
        ),
        body: Center(
          child: Keyboard.asset(
            name: widget.name,
            onSize: (size) {
              final ratio = MediaQuery.devicePixelRatioOf(context);
              Keebie.windowSize = Future.value(Size(
                size.width * ratio,
                (size.height + AppBar.preferredHeightFor(context, const Size.fromHeight(kToolbarHeight))) * ratio
              ));
            },
          ),
        ),
      );
}