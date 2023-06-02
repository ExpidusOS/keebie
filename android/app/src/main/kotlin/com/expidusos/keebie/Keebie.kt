package com.expidusos.keebie

import android.inputmethodservice.InputMethodService
import android.view.View

class Keebie: InputMethodService() {
    override fun onCreateInputView() : View {
        return layoutInflater.inflate(R.layout.keyboard_view, null).apply {
            requestLayout()
        }
    }

    override fun onCreateExtractTextView() : View? {
        return null
    }
}