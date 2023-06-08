package com.expidusos.keebie

import android.content.Context
import android.util.AttributeSet
import android.util.DisplayMetrics
import android.text.InputType
import android.view.inputmethod.EditorInfo
import android.view.ViewGroup

import io.flutter.embedding.android.FlutterSurfaceView
import io.flutter.embedding.android.FlutterView
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.embedding.engine.FlutterEngineGroup
import io.flutter.plugin.common.MethodChannel

class KeebieView(context: Context, attrs: AttributeSet) : ViewGroup(context, attrs) {
    val engineGroup = FlutterEngineGroup(context)
    var engine: FlutterEngine
    val surfaceView = FlutterSurfaceView(context)
    val view = FlutterView(context, surfaceView)
    var methodChannel: MethodChannel

    init {
        engine = engineGroup.createAndRunEngine(context, null, "/keyboard/${(context as Keebie).languageTag}")
        methodChannel = MethodChannel(engine.dartExecutor.binaryMessenger, "keebie")
        methodChannel.setMethodCallHandler {
            call, result ->
            when (call.method) {
                "sendKey" -> {
                    val type = call.argument("type") ?: "regular"
                    var name = call.argument("name") ?: ""
                    val shiftedName = call.argument("shiftedName") ?: ""
                    val isShifted = call.argument("isShifted") ?: false

                    if (isShifted && shiftedName.isNotEmpty()) {
                        name = shiftedName
                    }

                    val valid: Boolean = when (type) {
                      "backspace" -> {
                        (context as Keebie).currentInputConnection.deleteSurroundingText(1, 0)
                        true
                      }
                      "enter" -> {
                        (context as Keebie).currentInputConnection.performEditorAction(EditorInfo.IME_ACTION_GO)
                        true
                      }
                      "changeLang" -> (context as Keebie).switchToNextInputMethod(true)
                      "space" -> {
                        (context as Keebie).currentInputConnection.commitText(" ", 0)
                        true
                      }
                      "regular" -> {
                        (context as Keebie).currentInputConnection.commitText(name, 0)
                        true
                      }
                      else -> {
                          result.error(
                            "unknownKey",
                            "Key type $type is not a recognized key event type.",
                            call.arguments
                          )
                          false
                      }
                    }

                    if (valid) {
                      result.success(null)
                    } else {
                      result.error(type, "Failed to perform key action", call.arguments)
                    }
                }
                "announceLayout" -> result.success(null)
                "isKeyboard" -> result.success(true)
                "getContentType" -> {
                  val info = (context as Keebie).currentInputEditorInfo
                  result.success(when (info.inputType and InputType.TYPE_MASK_CLASS) {
                    InputType.TYPE_CLASS_DATETIME -> "dateTime"
                    InputType.TYPE_CLASS_NUMBER -> "number"
                    InputType.TYPE_CLASS_PHONE -> "phone"
                    InputType.TYPE_CLASS_TEXT -> "text"
                    else -> null
                  })
                }
                "getConstraints" -> {
                  val constraints = emptyList<String>().toMutableList()
                  val keebie = context as Keebie

                  if (keebie.shouldOfferSwitchingToNextInputMethod()) {
                    constraints += "canChangeLanguage"
                  }

                  result.success(constraints)
                }
                else -> result.notImplemented()
            }
        }

        surfaceView.attachToRenderer(engine.getRenderer())
        view.attachToFlutterEngine(engine)
        addView(view)
    }

    override fun onMeasure(width: Int, height: Int) {
        var displayMetrics = DisplayMetrics()
        context.display?.getMetrics(displayMetrics)

        var maxWidth = Math.max(width, displayMetrics.widthPixels)
        var maxHeight = Math.max(height, (displayMetrics.heightPixels / 3.15).toInt())

        measureChild(view, width, height)
        setMeasuredDimension(
            resolveSizeAndState(maxWidth, width, view.measuredState),
            resolveSizeAndState(maxHeight, height, view.measuredState shl MEASURED_HEIGHT_STATE_SHIFT)
        )
    }

    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        if (changed) view.layout(left, top, right, bottom)
    }
}