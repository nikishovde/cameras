package ru.visionlab.payment

import android.Manifest
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.graphics.SurfaceTexture
import android.hardware.Camera
import android.os.Bundle
import android.os.CountDownTimer
import android.renderscript.*
import android.util.Log
import android.view.*
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_main.*
import ru.visionlabs.payment.photo.presentation.ScriptC_rotation
import java.io.IOException
import java.nio.ByteBuffer
import kotlin.concurrent.thread


class MainActivity : AppCompatActivity(), Camera.PreviewCallback {

    companion object {
        private val REQUEST_CAMERA_PERMISSION = 1
        private val TAG = this.javaClass.canonicalName

        init {
            try {
//                System.loadLibrary("c++_shared");
                System.loadLibrary("wrapper");
//                System.loadLibrary("flower");
//                System.loadLibrary("FaceEngineSDK");
//                System.loadLibrary("TrackEngineSDK");
            } catch (e: UnsatisfiedLinkError) {
                Log.e("Luna Mobile", "Native library failed to load: $e");
                System.exit(1);
            }
        }
    }

    private lateinit var rotator: ScriptC_rotation
    private lateinit var rs: RenderScript

    private lateinit var yuvToRgbIntrinsic: ScriptIntrinsicYuvToRGB
    private lateinit var yuvToIrIntrinsic: ScriptIntrinsicYuvToRGB
    private lateinit var rsRgbInAllocation: Allocation
    private lateinit var rsRgbOutAllocation: Allocation
    private lateinit var rsRgbRotatedAllocation: Allocation
    private lateinit var previewRGBFrame: ByteArray
    private lateinit var rsIrInAllocation: Allocation
    private lateinit var rsIrOutAllocation: Allocation
    private lateinit var previewIrFrame: ByteArray

    private lateinit var cameraPreview: CameraPreview
    private lateinit var surfaceTextureListener: SurfaceTextureListener
    private val fakeSurfaceTexture by lazy {
        SurfaceTexture(1)
    }
    private val bestShotCallbackBuffer by lazy {
        ByteArray(640 * 640 * 3 / 2)
    }

    var irFrame = ByteBuffer.allocateDirect(640 * 640 * 4)
    val rgbFrame = ByteBuffer.allocateDirect(640 * 640 * 4)
    var irFrameCopyDataSemaphor = true

    var irCamera: Camera? = null
    var rgbCamera: Camera? = null
    lateinit var camera_surface: SurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        camera_surface = findViewById(R.id.camera_surface)
        rs = RenderScript.create(this)
        rotator = ScriptC_rotation(rs)
        rsSetup()

//        unpackResourcesAndInitFaceEngine()
        requestCameraPermission()
        resume.setOnClickListener(object : View.OnClickListener {
            override fun onClick(v: View?) {
                resume()
//                autoPause()
            }
        })
        pause.setOnClickListener(object : View.OnClickListener {
            override fun onClick(v: View?) {
                pause()
//                autoResume()
            }

        })
//        cputOverload()
    }

    fun requestCameraPermission() {
        requestPermissions(
            arrayOf(Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE),
            REQUEST_CAMERA_PERMISSION
        )
    }

    override fun onDestroy() {
        super.onDestroy()
        pause()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        when (requestCode) {
            REQUEST_CAMERA_PERMISSION -> {
                if (grantResults.size > 0
                    && grantResults[0] == PackageManager.PERMISSION_GRANTED
                    && grantResults[1] == PackageManager.PERMISSION_GRANTED
                ) {

                    rgbCamera = getCameraInstance(1)!!
                    configureCamera(rgbCamera!!)
                    configureRgbCameraPreview()
                    rgbCamera!!.setPreviewCallback(this)

                    irCamera = getCameraInstance(0)!!
                    configureCamera(irCamera!!)
                    startIrCameraPreview()
                    irCamera!!.setPreviewTexture(fakeSurfaceTexture)
                    irCamera!!.startPreview()
                    irCameraDataCallback()

//                    autoPause()

//                    pushFromFiles()
                } else {
                }
                return
            }
        }
    }


    fun getCameraInstance(cameraId: Int): Camera? {
        return try {
            Camera.open(cameraId) // attempt to get a Camera instance
        } catch (e: Exception) {
            // Camera is not available (in use or does not exist)
            null // returns null if camera is unavailable
        }
    }

    private fun configureCamera(camera: Camera) {
        camera.parameters?.apply {
            setPreviewSize(640, 480)
        }?.also {
            camera.apply {
                this.parameters = it
            }
        }
    }

    private fun startIrCameraPreview() {
        irCamera?.run {
            stopPreview()
            setPreviewTexture(fakeSurfaceTexture)
            startPreview()
        }
    }

    private fun startBestshotCameraPreview(holder: SurfaceTexture) {
        rgbCamera?.run {
            stopPreview()
            setPreviewTexture(holder)
            addCallbackBuffer(bestShotCallbackBuffer)
            setPreviewCallbackWithBuffer(this@MainActivity)
            startPreview()
        }
    }

    private fun configureRgbCameraPreview() {
        cameraPreview = CameraPreview(this, rgbCamera!!)
            .apply {
                layoutParams = FrameLayout.LayoutParams(
                    FrameLayout.LayoutParams.MATCH_PARENT,
                    FrameLayout.LayoutParams.MATCH_PARENT
                ).apply {
                    gravity = Gravity.CENTER
                }
            }

        cameraPreview.scaleX = -1.0f
//        cameraPreview.rotation = -90f
        preview_container.removeAllViews()
        preview_container.addView(cameraPreview)

        val viewTreeObserver = preview_container.viewTreeObserver
        viewTreeObserver?.addOnGlobalLayoutListener(object :
            ViewTreeObserver.OnGlobalLayoutListener {
            override fun onGlobalLayout() {
                preview_container.viewTreeObserver.removeGlobalOnLayoutListener(this)
                // holderCallback = HolderCallback()
                surfaceTextureListener = SurfaceTextureListener()
                cameraPreview.surfaceTextureListener = surfaceTextureListener
            }
        })
    }

    private fun irCameraDataCallback() {
        irCamera?.setPreviewCallback { data, camera ->
            Log.d(this.javaClass.canonicalName, "Ir camera data size = ${data.size}")
            return@setPreviewCallback
//            rsIrInAllocation.copyFrom(data)
//            yuvToIrIntrinsic.forEach(rsIrOutAllocation)
//            rsIrOutAllocation.copyTo(previewIrFrame)
//
            if (irFrameCopyDataSemaphor) {
                rsIrInAllocation.copyFrom(data)
                yuvToIrIntrinsic.forEach(rsIrOutAllocation)
                rsIrOutAllocation.copyTo(previewIrFrame)
                irFrame.clear()
                irFrame.put(previewIrFrame)

                irFrameCopyDataSemaphor = false
            }
        }
    }

    private fun rgbCameraDataCallback() {
        rgbCamera?.setPreviewCallback { data, camera ->
            Log.d(this.javaClass.canonicalName, "Rgb camera data size = ${data.size}")
        }
    }

    private inner class SurfaceTextureListener : TextureView.SurfaceTextureListener {
        override fun onSurfaceTextureSizeChanged(
            surface: SurfaceTexture,
            width: Int,
            height: Int
        ) {
        }

        override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {
        }

        override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
            return true
        }

        override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
            rgbCamera?.run {
                try {
                    setPreviewTexture(surface)
                    startBestshotCameraPreview(surface!!)
                } catch (e: IOException) {
                    e.printStackTrace()
                }
            }
        }

    }

    override fun onPreviewFrame(data: ByteArray?, camera: Camera?) {
        Log.d(this.javaClass.canonicalName, "Rgb camera data size = ${data!!.size}")

        camera?.addCallbackBuffer(bestShotCallbackBuffer)
        return

        rsRgbInAllocation.copyFrom(data)
        yuvToRgbIntrinsic.forEach(rsRgbOutAllocation)
//        rotator._gCurrentFrame = rsRgbOutAllocation
//        rotator._inWidth = 480
//        rotator._inHeight = 640
//        rotator.forEach_rotate_90_counterclockwise(rsRgbOutAllocation, rsRgbRotatedAllocation)
        rsRgbOutAllocation.copyTo(previewRGBFrame)

        rgbFrame.clear()
        rgbFrame.put(previewRGBFrame)
        if (!irFrameCopyDataSemaphor) {
            pushFrame(rgbFrame, irFrame, System.currentTimeMillis(), save.isChecked)
//
//            thread(start = true) {
//            pushByteBuffer(previewRGBFrame, previewIrFrame, System.currentTimeMillis())
//            }
            irFrameCopyDataSemaphor = true
        }

        camera?.addCallbackBuffer(bestShotCallbackBuffer)
    }

    fun pause() {
        rgbCamera!!.setPreviewCallback(null)
        rgbCamera!!.stopPreview()
        rgbCamera!!.release()
        rgbCamera

        irCamera!!.setPreviewCallback(null)
        irCamera!!.stopPreview()
        irCamera!!.release()
        irCamera = null
    }

    fun resume() {
//        resetCounter()
        irCamera = getCameraInstance(1)!!
        configureCamera(irCamera!!)
        startIrCameraPreview()
        irCameraDataCallback()

        rgbCamera = getCameraInstance(0)!!
        configureCamera(rgbCamera!!)
        configureRgbCameraPreview()
    }

    fun autoPause() {
        val timer = object : CountDownTimer(40000, 1000) {
            override fun onTick(millisUntilFinished: Long) {

            }

            override fun onFinish() {
                pause()
                autoResume()
            }
        }
        timer.start()
    }

    fun autoResume() {
        val timer = object : CountDownTimer(10000, 1000) {
            override fun onTick(millisUntilFinished: Long) {

            }

            override fun onFinish() {
                resume()
                autoPause()
            }
        }
        timer.start()
    }

    fun cputOverload() {
        var i = 0
        thread(start = true)
        {
            while(true) {
                i++
            }
        }
    }

    fun unpackResourcesAndInitFaceEngine(): Int {

        if (!Utils.createVLDataFolder(this)) {
            return -1
        }

        val dataAssetsUnpackedSuccess = Utils.createFilesFromAssetFolder(this, "data")
        Log.d(TAG, "ASSETS UNPACKED: $dataAssetsUnpackedSuccess")

        if (!dataAssetsUnpackedSuccess) {
            Log.e(TAG, "Failed to unpack resources from asset folder!")
            return -1
        }

        val initCode = initFaceEngine("$filesDir/vl/data",
            0.843f, 20f, 20f, 10f)
        Log.d(TAG, "RETURN CODE IS $initCode")
        return initCode
    }

    fun rsSetup() {
        val rgbArraySize: Int = 640 * 640 * 4

        val nv21Type = Type.Builder(rs, Element.U8(rs))
            .setX(640)
            .setY(640)
            .setYuvFormat(ImageFormat.NV21)
            .create()
        val rgbType = Type.Builder(rs, Element.RGBA_8888(rs))
            .setX(640)
            .setY(640)
            .create()

        yuvToRgbIntrinsic = ScriptIntrinsicYuvToRGB.create(rs, Element.U8_4(rs))
        rsRgbInAllocation = Allocation.createTyped(rs, nv21Type)
        rsRgbOutAllocation = Allocation.createTyped(rs, rgbType)
        rsRgbRotatedAllocation = Allocation.createTyped(rs, rgbType, Allocation.USAGE_SCRIPT)
        previewRGBFrame = ByteArray(rgbArraySize)
        yuvToRgbIntrinsic.setInput(rsRgbInAllocation)

        yuvToIrIntrinsic = ScriptIntrinsicYuvToRGB.create(rs, Element.U8_4(rs))
        rsIrInAllocation = Allocation.createTyped(rs, nv21Type)
        rsIrOutAllocation = Allocation.createTyped(rs, rgbType)
        previewIrFrame  = ByteArray(rgbArraySize)
        yuvToIrIntrinsic.setInput(rsIrInAllocation)
    }

    private external fun pushFrame(rgbFrame: ByteBuffer, irFrame: ByteBuffer, frameTimestamp: Long, saveRawPhotos: Boolean )
    private external fun pushFromFiles()
    private external fun pushByteBuffer(rgbFrame: ByteArray , irFrame: ByteArray, frameTimestamp: Long )
    private external fun initFaceEngine(
        path: String?,
        livenessThreshold: Float,
        yawThreshold: Float,
        pitchThreshold: Float,
        rollThreshold: Float
    ): Int
    private external fun resetCounter()
}