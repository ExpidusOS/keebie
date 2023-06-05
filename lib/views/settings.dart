import 'package:bitsdojo_window/bitsdojo_window.dart';
import 'package:flutter/foundation.dart';
import 'package:keebie/constants.dart';
import 'package:keebie/logic.dart';
import 'package:keebie/views.dart';
import 'package:keebie/main.dart';
import 'package:libtokyo_flutter/libtokyo.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:shared_preferences/shared_preferences.dart';

class SettingsView extends StatefulWidget {
  const SettingsView({ super.key });

  @override
  State<SettingsView> createState() => _SettingsViewState();
}

class _SettingsViewState extends State<SettingsView> {
  late SharedPreferences preferences;
  bool optInErrorReporting = false;

  @override
  void initState() {
    super.initState();

    SharedPreferences.getInstance().then((prefs) => setState(() {
      preferences = prefs;
      _loadSettings();
    })).catchError((error, trace) => handleError(error, trace: trace));
  }

  void _loadSettings() {
    optInErrorReporting = KeebieSettings.optInErrorReporting.valueFor(preferences);
  }

  void _handleError(BuildContext context, Object e) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(AppLocalizations.of(context)!.genericErrorMessage(e.toString())),
        duration: const Duration(milliseconds: 1500),
        behavior: SnackBarBehavior.floating,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(10.0),
        ),
      )
    );
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
        appBar: AppBar(
          title: Text(AppLocalizations.of(context)!.viewSettings),
        ),
        body: ListView(
          children: [
            ...(const String.fromEnvironment('SENTRY_DSN', defaultValue: '').isNotEmpty ? [
              SwitchListTile(
                title: Text(AppLocalizations.of(context)!.settingsOptInErrorReporting),
                subtitle: Text(AppLocalizations.of(context)!.settingsOptInErrorReportingSubtitle),
                value: optInErrorReporting,
                onChanged: (value) => preferences.setBool(KeebieSettings.optInErrorReporting.name, value).then((value) {
                  setState(() {
                    optInErrorReporting = value;
                  });
                }).catchError((error) {
                  _handleError(context, error);
                }),
              ),
            ] : []),
            ListTile(
              title: Text(AppLocalizations.of(context)!.settingsRestoreDefaults),
              onTap: () => preferences.clear().then((value) => setState(() {
                _loadSettings();
              })).catchError((error) => _handleError(context, error)),
            ),
            const Divider(),
            ListTile(
              title: Text(AppLocalizations.of(context)!.viewAbout),
              onTap: () => Navigator.of(context).push(
                  MaterialPageRoute(
                    builder: (context) => const About(),
                    settings: const RouteSettings(name: 'About'),
                  )
              ),
            ),
          ].map((child) => child is Divider ? child : ListTileTheme(
            tileColor: Theme.of(context).cardTheme.color ?? Theme.of(context).cardColor,
            shape: Theme.of(context).cardTheme.shape,
            contentPadding: Theme.of(context).cardTheme.margin,
            child: child
          )).toList(),
        ),
      );
}