import 'package:bitsdojo_window/bitsdojo_window.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:libtokyo_flutter/libtokyo.dart' hide ColorScheme;
import 'package:libtokyo/libtokyo.dart' show ColorScheme;
import 'package:pubspec/pubspec.dart';
import 'package:sentry_flutter/sentry_flutter.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'constants.dart';
import 'logic.dart';
import 'views.dart';

Future<Size> _getWindowSize() async {
  try {
    final value = await const MethodChannel('keebie').invokeMethod('getMonitorGeometry');
    return Size(value['width']!.toDouble(), value['height']! / 3.15);
  } catch (e) {
    return const Size(600, 450);
  }
}

Future<bool> _isKeyboard() async {
  try {
    return await const MethodChannel('keebie').invokeMethod('isKeyboard');
  } catch (e) {
    return false;
  }
}

Future<void> _runMain({
  required bool isSentry,
  required PubSpec pubspec,
  required List<String> args,
}) async {
  final isKeyboard = await _isKeyboard();

  final app = KeebieApp(
    isSentry: isSentry,
    isKeyboard: isKeyboard,
    pubspec: pubspec
  );
  runApp(isSentry ? DefaultAssetBundle(bundle: SentryAssetBundle(), child: app) : app);

  switch (defaultTargetPlatform) {
    case TargetPlatform.windows:
    case TargetPlatform.macOS:
    case TargetPlatform.linux:
      final initialSize = isKeyboard ? await _getWindowSize() : const Size(600, 450);
      doWhenWindowReady(() {
        final win = appWindow;

        win.minSize = initialSize;
        win.size = initialSize;
        win.alignment = Alignment.center;
        win.show();
      });
      break;
    default:
      break;
  }
}

Future<void> main(List<String> args) async {
  WidgetsFlutterBinding.ensureInitialized();
  final pubspec = PubSpec.fromYamlString(await rootBundle.loadString('pubspec.yaml'));

  const sentryDsn = String.fromEnvironment('SENTRY_DSN', defaultValue: '');

  if (sentryDsn.isNotEmpty && await KeebieSettings.optInErrorReporting.value) {
    await SentryFlutter.init(
      (options) {
        options.dsn = sentryDsn;
        options.tracesSampleRate = 1.0;
        options.release = 'com.expidusos.keebie@${pubspec.version!}';

        if (kDebugMode) {
          options.environment = 'debug';
        } else if (kProfileMode) {
          options.environment = 'profile';
        } else if (kReleaseMode) {
          options.environment = 'release';
        }
      },
      appRunner: () => _runMain(
        isSentry: true,
        pubspec: pubspec,
        args: args,
      ).catchError((error, trace) => handleError(error, trace: trace)),
    );
  } else {
    await _runMain(
      isSentry: false,
      pubspec: pubspec,
      args: args
    );
  }
}

class KeebieApp extends StatefulWidget {
  const KeebieApp({
    super.key,
    required this.isSentry,
    required this.isKeyboard,
    required this.pubspec
  });

  final bool isSentry;
  final bool isKeyboard;
  final PubSpec pubspec;

  @override
  State<KeebieApp> createState() => _KeebieAppState();

  static Future<void> reload(BuildContext context) => context.findAncestorStateOfType<_KeebieAppState>()!.reload();
  static bool isSentryOnContext(BuildContext context) => context.findAncestorWidgetOfExactType<KeebieApp>()!.isSentry;
  static PubSpec getPubSpec(BuildContext context) => context.findAncestorWidgetOfExactType<KeebieApp>()!.pubspec;
}

class _KeebieAppState extends State<KeebieApp> {
  late SharedPreferences preferences;
  ColorScheme? colorScheme;

  @override
  void initState() {
    super.initState();

    SharedPreferences.getInstance().then((prefs) => setState(() {
      preferences = prefs;
      _loadSettings();
    })).catchError((error, trace) {
      handleError(error, trace: trace);
    });
  }

  void _loadSettings() {
    colorScheme = KeebieSettings.colorScheme.valueFor(preferences);
  }

  Future<void> reload() async {
    await preferences.reload();
    setState(() => _loadSettings());
  }

  @override
  Widget build(BuildContext context) =>
    TokyoApp(
      colorScheme: colorScheme,
      onGenerateTitle: (context) => AppLocalizations.of(context)!.applicationTitle,
      localizationsDelegates: AppLocalizations.localizationsDelegates,
      supportedLocales: AppLocalizations.supportedLocales,
      initialRoute: widget.isKeyboard ? '/keyboard' : '/settings',
      navigatorObservers: widget.isSentry ? [
        SentryNavigatorObserver(
          setRouteNameAsTransaction: true,
        ),
      ] : null,
      routes: {
        '/settings': (context) => const SettingsView(),
        '/keyboard/en-US': (context) => const KeyboardView(name: 'en-US'),
        '/keyboard/ja-JP': (context) => const KeyboardView(name: 'ja-JP'),
        '/keyboard': (context) {
          final locale = Localizations.localeOf(context);
          var name = locale.languageCode;

          if (locale.countryCode != null) {
            name += '-${locale.countryCode}';
          } else {
            switch (locale.languageCode) {
              case 'ja':
                name += '-JP';
                break;
              case 'en':
                name += '-US';
                break;
            }
          }
          return KeyboardView(name: name);
        }
      },
    );
}
