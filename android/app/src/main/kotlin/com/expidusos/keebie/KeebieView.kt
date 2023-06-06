package com.expidusos.keebie

import android.content.Context
import android.os.SystemClock
import android.util.AttributeSet
import android.util.DisplayMetrics
import android.view.KeyEvent
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
                    var valid = true
                    val meta = if (isShifted) KeyEvent.META_SHIFT_MASK else 0

                    if (isShifted && shiftedName.isNotEmpty()) {
                        name = shiftedName.lowercase()
                    }

                    val code: Int = when (type) {
                        "backspace" -> KeyEvent.KEYCODE_DEL
                        "enter" -> KeyEvent.KEYCODE_ENTER
                        "space" -> KeyEvent.KEYCODE_SPACE
                        "regular" -> when (name) {
                            "1" -> KeyEvent.KEYCODE_1
                            "2" -> KeyEvent.KEYCODE_2
                            "3" -> KeyEvent.KEYCODE_3
                            "4" -> KeyEvent.KEYCODE_4
                            "5" -> KeyEvent.KEYCODE_5
                            "6" -> KeyEvent.KEYCODE_6
                            "7" -> KeyEvent.KEYCODE_7
                            "8" -> KeyEvent.KEYCODE_8
                            "9" -> KeyEvent.KEYCODE_9
                            "0" -> KeyEvent.KEYCODE_0
                            "a" -> KeyEvent.KEYCODE_A
                            "b" -> KeyEvent.KEYCODE_B
                            "c" -> KeyEvent.KEYCODE_C
                            "d" -> KeyEvent.KEYCODE_D
                            "e" -> KeyEvent.KEYCODE_E
                            "f" -> KeyEvent.KEYCODE_F
                            "g" -> KeyEvent.KEYCODE_G
                            "h" -> KeyEvent.KEYCODE_H
                            "i" -> KeyEvent.KEYCODE_I
                            "j" -> KeyEvent.KEYCODE_J
                            "k" -> KeyEvent.KEYCODE_K
                            "l" -> KeyEvent.KEYCODE_L
                            "m" -> KeyEvent.KEYCODE_M
                            "n" -> KeyEvent.KEYCODE_N
                            "o" -> KeyEvent.KEYCODE_O
                            "p" -> KeyEvent.KEYCODE_P
                            "q" -> KeyEvent.KEYCODE_Q
                            "r" -> KeyEvent.KEYCODE_R
                            "s" -> KeyEvent.KEYCODE_S
                            "t" -> KeyEvent.KEYCODE_T
                            "u" -> KeyEvent.KEYCODE_U
                            "v" -> KeyEvent.KEYCODE_V
                            "w" -> KeyEvent.KEYCODE_W
                            "x" -> KeyEvent.KEYCODE_X
                            "y" -> KeyEvent.KEYCODE_Y
                            "z" -> KeyEvent.KEYCODE_Z
                            "." -> KeyEvent.KEYCODE_PERIOD
                            "," -> KeyEvent.KEYCODE_COMMA
                            else -> {
                                result.error("regularKey", "Key $name is not a recognized regular key event.", call.arguments)
                                valid = false
                                0
                            }
                        }
                        else -> {
                            result.error("unknownKey", "Key type $type is not a recognized key event type.", call.arguments)
                            valid = false
                            0
                        }
                    }

                    if (valid) {
                        val time = SystemClock.uptimeMillis()
                        println(KeyEvent(time, time, KeyEvent.ACTION_UP, code, 0, meta).getMetaState())
                        (context as Keebie).currentInputConnection.sendKeyEvent(KeyEvent(time, time, KeyEvent.ACTION_UP, code, 0, meta))
                        (context as Keebie).currentInputConnection.sendKeyEvent(KeyEvent(time, time, KeyEvent.ACTION_DOWN, code, 0, meta))
                        result.success(null)
                    }
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