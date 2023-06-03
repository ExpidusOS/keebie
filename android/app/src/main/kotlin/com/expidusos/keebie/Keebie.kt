package com.expidusos.keebie

import android.inputmethodservice.InputMethodService
import android.view.inputmethod.InputMethodManager
import android.view.inputmethod.InputMethodSubtype
import android.util.DisplayMetrics
import android.view.View

class Keebie: InputMethodService() {
    val languageTag: String
        get() = (getSystemService(InputMethodService.INPUT_METHOD_SERVICE) as InputMethodManager).currentInputMethodSubtype.languageTag

    lateinit var inputView: KeebieView

    override fun onCreateInputView() : View {
        return layoutInflater.inflate(R.layout.keyboard_view, null).apply {
            requestLayout()
            (this@Keebie).updateFullscreenMode()
            (this@Keebie).inputView = this as KeebieView
        }
    }

    override fun onCurrentInputMethodSubtypeChanged(subtype: InputMethodSubtype) {
        inputView.engine.navigationChannel.popRoute()
        inputView.engine.navigationChannel.pushRoute("/keyboard/${subtype.languageTag}")
    }

    override fun onWindowShown() {
        inputView.engine.lifecycleChannel.appIsResumed()
    }

    override fun onWindowHidden() {
        inputView.engine.lifecycleChannel.appIsInactive()
    }

    override fun onCreateExtractTextView() : View? {
        return null
    }

    override fun onEvaluateFullscreenMode() : Boolean {
        return false
    }

    override fun isExtractViewShown(): Boolean {
        return false
    }
}