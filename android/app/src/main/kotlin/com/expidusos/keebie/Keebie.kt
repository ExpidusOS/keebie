package com.expidusos.keebie

import android.inputmethodservice.InputMethodService
import android.util.DisplayMetrics
import android.view.View

class Keebie: InputMethodService() {
    override fun onCreateInputView() : View {
        return layoutInflater.inflate(R.layout.keyboard_view, null).apply {
            requestLayout()
            (this@Keebie).updateFullscreenMode()
        }
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