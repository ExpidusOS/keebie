import 'package:flutter/foundation.dart';
import 'package:keebie/constants.dart';
import 'package:keebie/logic.dart';
import 'package:keebie/main.dart';
import 'package:keebie/views.dart';
import 'package:libtokyo/logic/theme.dart';
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
  ColorScheme colorScheme = ColorScheme.night;

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
    colorScheme = ColorScheme.values.asNameMap()[preferences.getString(KeebieSettings.colorScheme.name) ?? 'night']!;
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
        windowBar: WindowBar.shouldShow(context) && !kIsWeb ? WindowBar(
          leading: Image.asset('assets/imgs/icon.png'),
          title: Text(AppLocalizations.of(context)!.applicationTitle),
        ) : null,
        appBar: AppBar(
          title: Text(AppLocalizations.of(context)!.viewSettings),
        ),
        body: ListView(
          children: [
            ListTile(
              title: Text(AppLocalizations.of(context)!.settingsTheme),
              onTap: () =>
                showDialog(
                  context: context,
                  builder: (context) =>
                      Dialog(
                        child: Padding(
                          padding: const EdgeInsets.all(8.0),
                          child: ListView(
                            children: [
                              RadioListTile(
                                title: const Text('Storm'),
                                value: ColorScheme.storm,
                                groupValue: colorScheme,
                                onChanged: (value) =>
                                    preferences.setString(
                                        KeebieSettings.colorScheme.name,
                                        value!.name).then((v) {
                                      setState(() {
                                        colorScheme = value;
                                        KeebieApp.reload(context);
                                      });
                                    }).catchError((error) {
                                      _handleError(context, error);
                                    }),
                              ),
                              RadioListTile(
                                title: const Text('Night'),
                                value: ColorScheme.night,
                                groupValue: colorScheme,
                                onChanged: (value) =>
                                    preferences.setString(
                                        KeebieSettings.colorScheme.name,
                                        value!.name).then((v) {
                                      setState(() {
                                        colorScheme = value;
                                        KeebieApp.reload(context);
                                      });
                                    }).catchError((error) {
                                      _handleError(context, error);
                                    }),
                              ),
                              RadioListTile(
                                title: const Text('Moon'),
                                value: ColorScheme.moon,
                                groupValue: colorScheme,
                                onChanged: (value) =>
                                    preferences.setString(
                                        KeebieSettings.colorScheme.name,
                                        value!.name).then((v) {
                                      setState(() {
                                        colorScheme = value;
                                        KeebieApp.reload(context);
                                      });
                                    }).catchError((error) {
                                      _handleError(context, error);
                                    }),
                              ),
                              RadioListTile(
                                title: const Text('Day'),
                                value: ColorScheme.day,
                                groupValue: colorScheme,
                                onChanged: (value) =>
                                    preferences.setString(
                                        KeebieSettings.colorScheme.name,
                                        value!.name).then((v) {
                                      setState(() {
                                        colorScheme = value;
                                        KeebieApp.reload(context);
                                      });
                                    }).catchError((error) {
                                      _handleError(context, error);
                                    }),
                              )
                            ],
                          ),
                        ),
                      ),
                ),
            ),
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
                KeebieApp.reload(context);
                Keebie.announceSettingsChange();
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