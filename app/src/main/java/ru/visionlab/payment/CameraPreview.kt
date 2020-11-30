package ru.visionlab.payment

import android.annotation.SuppressLint
import android.content.Context
import android.hardware.Camera
import android.hardware.Camera.Size
import android.view.TextureView

@SuppressLint("ViewConstructor")
class CameraPreview(private val mContext: Context, private val mCamera: Camera) : TextureView(mContext) {
    private var mPreviewSize: Size? = null

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = 640
        val height = 640
        val ratio: Float
        ratio = if (height >= width) {
            height.toFloat() / width.toFloat()
        } else {
            width.toFloat() / height.toFloat()
        }

        // One of these methods should be used, second method squishes preview slightly
        // setMeasuredDimension(width, (int) (width * ratio));
        setMeasuredDimension((height * ratio).toInt(), height)
    }

    companion object {
        private const val TAG = "CameraPreview"
    }

}