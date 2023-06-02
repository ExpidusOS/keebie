package com.expidusos.keebie

import android.content.Context
import android.util.AttributeSet
import android.util.DisplayMetrics
import android.view.ViewGroup

import io.flutter.FlutterInjector
import io.flutter.embedding.android.FlutterSurfaceView
import io.flutter.embedding.android.FlutterView
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.embedding.engine.FlutterEngineGroup

class KeebieView(context: Context, attrs: AttributeSet) : ViewGroup(context, attrs) {
    val engineGroup = FlutterEngineGroup(context)
    var engine: FlutterEngine
    val surfaceView = FlutterSurfaceView(context)
    val view = FlutterView(context, surfaceView)

    init {
        engine = engineGroup.createAndRunEngine(context, null, "/keyboard")
        surfaceView.attachToRenderer(engine.getRenderer())
        view.attachToFlutterEngine(engine)
        addView(view)
    }

    override fun onMeasure(width: Int, height: Int) {
        var displayMetrics = DisplayMetrics()
        context.display?.getMetrics(displayMetrics)

        var maxWidth = Math.max(width, displayMetrics.widthPixels)
        var maxHeight = Math.max(height, (displayMetrics.heightPixels / 1.5).toInt())

        println("${maxWidth}x${maxHeight}")

        measureChild(view, width, height)
        setMeasuredDimension(
            resolveSizeAndState(maxWidth, width, view.measuredState),
            resolveSizeAndState(maxHeight, height, view.measuredState)
        )
    }

    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        view.layout(left, top, right, bottom)
    }
}