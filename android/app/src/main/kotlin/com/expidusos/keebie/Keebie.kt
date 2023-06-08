package com.expidusos.keebie

import android.inputmethodservice.InputMethodService
import android.view.inputmethod.InputMethodManager
import android.view.inputmethod.InputMethodInfo
import android.view.inputmethod.InputMethodSubtype
import android.view.View

class Keebie: InputMethodService() {
    private var _languageTag: String? = null

    var languageTag: String
        get() = _languageTag ?: (getSystemService(InputMethodService.INPUT_METHOD_SERVICE) as InputMethodManager).currentInputMethodSubtype.languageTag
        set(value) {
            if (!switchToNextInputMethod(true)) {
              inputView.engine.navigationChannel.popRoute()
              inputView.engine.navigationChannel.pushRoute("/keyboard/${value}")
              _languageTag = value
            }
        }

    val inputMethodInfo: InputMethodInfo
        get() = ((getSystemService(InputMethodService.INPUT_METHOD_SERVICE) as InputMethodManager).inputMethodList.find {
          method -> method.id.contains("keebie")
        })!!

    val nextInputMethodSubtype: InputMethodSubtype?
        get() {
            for (i in 0..inputMethodInfo.subtypeCount) {
                val subtype = inputMethodInfo.getSubtypeAt(i);
                if (subtype.languageTag != languageTag) return subtype
            }
            return null
        }

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