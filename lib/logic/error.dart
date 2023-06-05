import 'package:keebie/constants.dart';
import 'package:sentry_flutter/sentry_flutter.dart';
import 'package:flutter/foundation.dart';

Future<bool> isSentryEnabled() async {
  try {
    const sentryDsn = String.fromEnvironment('SENTRY_DSN', defaultValue: '');
    return sentryDsn.isNotEmpty && await KeebieSettings.optInErrorReporting.value;
  } catch (error, trace) {
    FlutterError.reportError(FlutterErrorDetails(
      exception: error,
      stack: trace,
    ));
    return false;
  }
}

void handleError(Object e, { StackTrace? trace, bool sendFlutter = !kReleaseMode, }) {
  trace ??= StackTrace.current;

  isSentryEnabled().then((isSentry) {
    if (isSentry) {
      Sentry.captureException(
        e,
        stackTrace: trace,
      ).catchError((error, trace) {
        FlutterError.reportError(FlutterErrorDetails(
          exception: error,
          stack: trace,
        ));
      });

      if (!sendFlutter) {
        return;
      }
    }

    FlutterError.reportError(FlutterErrorDetails(
      exception: e,
      stack: trace,
    ));
  });
}