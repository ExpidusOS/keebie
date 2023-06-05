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

Future<void> _runMain({
  required bool isSentry,
  required PubSpec pubspec,
  required List<String> args,
}) async {
  final app = KeebieApp(isSentry: isSentry, pubspec: pubspec);
  runApp(isSentry ? DefaultAssetBundle(bundle: SentryAssetBundle(), child: app) : app);

  switch (defaultTargetPlatform) {
    case TargetPlatform.windows:
    case TargetPlatform.macOS:
    case TargetPlatform.linux:
      doWhenWindowReady(() {
        final win = appWindow;
        const initialSize = Size(600, 450);

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
  const KeebieApp({ super.key, required this.isSentry, required this.pubspec });

  final bool isSentry;
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
      navigatorObservers: widget.isSentry ? [
        SentryNavigatorObserver(
          setRouteNameAsTransaction: true,
        ),
      ] : null,
      routes: {
        '/settings': (context) => const SettingsView(),
        '/keyboard/en-US': (context) => const KeyboardView(name: 'en-US'),
        '/keyboard/ja-JP': (context) => const KeyboardView(name: 'ja-JP'),
      },
    );
}
