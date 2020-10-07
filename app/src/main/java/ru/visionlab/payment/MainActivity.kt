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
import android.widget.Button
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_main.*
import java.io.IOException
import java.nio.ByteBuffer
import kotlin.concurrent.thread


class MainActivity : AppCompatActivity(), Camera.PreviewCallback {

    companion object {
        private val REQUEST_CAMERA_PERMISSION = 1
        private val TAG = this.javaClass.canonicalName
        private val RGB_CAMERA_ID = 0
        private val IR_CAMERA_IO = 1

//        init {
//            try {
//                System.loadLibrary("c++_shared");
//                System.loadLibrary("wrapper");
//                System.loadLibrary("flower");
//                System.loadLibrary("FaceEngineSDK");
//                System.loadLibrary("TrackEngineSDK");
//            } catch (e: UnsatisfiedLinkError) {
//                Log.e("Luna Mobile", "Native library failed to load: $e");
//                System.exit(1);
//            }
//        }
    }

    private var currentCamera = 0
    private lateinit var rs: RenderScript

    private lateinit var yuvToRgbIntrinsic: ScriptIntrinsicYuvToRGB
    private lateinit var yuvToIrIntrinsic: ScriptIntrinsicYuvToRGB
    private lateinit var rsRgbInAllocation: Allocation
    private lateinit var rsRgbOutAllocation: Allocation
    private lateinit var previewRGBFrame: ByteArray
    private lateinit var previewRGBFrameRotated: ByteArray
    private lateinit var rsIrInAllocation: Allocation
    private lateinit var rsIrOutAllocation: Allocation
    private lateinit var previewIrFrame: ByteArray

    private lateinit var cameraPreview: CameraPreview
    private lateinit var surfaceTextureListener: SurfaceTextureListener
    private val fakeSurfaceTexture by lazy {
        SurfaceTexture(1)
    }
    private val bestShotCallbackBuffer by lazy {
        ByteArray(640 * 480 * 3 / 2)
    }

    var irFrame = ByteBuffer.allocateDirect(640 * 480 * 4)
    val rgbFrame = ByteBuffer.allocateDirect(640 * 480 * 4)
    var irFrameCopyDataSemaphor = true

    var irCamera: Camera? = null
    var rgbCamera: Camera? = null
    lateinit var camera_surface: SurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        camera_surface = findViewById(R.id.camera_surface)
        rs = RenderScript.create(this)
        rsSetup()

//        unpackResourcesAndInitFaceEngine()
        requestCameraPermission()

//        cputOverload()
        setupListeners()
    }

    private fun setupListeners() {
        this.findViewById<Button>(R.id.rgbCamera).setOnClickListener {
            pause()
            currentCamera = RGB_CAMERA_ID
            startRgb()
        }
        this.findViewById<Button>(R.id.irCamera).setOnClickListener {
            pause()
            currentCamera = IR_CAMERA_IO
            startIr()
        }
        this.findViewById<Button>(R.id.detect).setOnClickListener {

//            if (detecting.visibility == View.GONE) {
//                CoroutineScope(Dispatchers.Default).launch() {
//                    pause()
//                    currentCamera = RGB_CAMERA_ID
//                    rgbCamera = getCameraInstance(RGB_CAMERA_ID)!!
//                    configureCamera(rgbCamera!!)
////        configureRgbCameraPreview()
//                    rgbCamera!!.setPreviewDisplay(camera_surface.holder)
////        rgbCamera!!.setDisplayOrientation(270)
//                    rgbCamera!!.startPreview()
//                    takePhoto()
//                    sleep(3000)
//                    pause()
//                    currentCamera = IR_CAMERA_IO
//                    startIr()
//                    takePhoto()
//                    sleep(3000)
//                    pause()
//                    currentCamera = RGB_CAMERA_ID
//                    startRgb()
//
//                    startDetecting()
//                }
//                detecting.visibility = View.VISIBLE
//            } else {
//                stopDetecting()
//                detecting.visibility = View.GONE
//            }
        }
//        resume.setOnClickListener(object : View.OnClickListener {
//            override fun onClick(v: View?) {
//                resume()
//                autoPause()
//            }
//        })
//        pause.setOnClickListener(object : View.OnClickListener {
//            override fun onClick(v: View?) {
//                pause()
//                autoResume()
//            }
//
//        })
    }

    private fun takePhoto() {
        if (currentCamera == RGB_CAMERA_ID && rgbCamera != null) {
            rgbCamera!!.setPreviewCallback { data, camera ->

                Log.d(this.javaClass.canonicalName, "rgb camera data size = ${data.size}")
                rsRgbInAllocation.copyFrom(data)
                yuvToRgbIntrinsic.forEach(rsRgbOutAllocation)
                rsRgbOutAllocation.copyTo(previewRGBFrame)

                rgbFrame.put(previewRGBFrame)
//                saveFrame(rgbFrame, currentCamera)
                camera.setPreviewCallback(null)
//                rgbFrame.clear()
            }
        } else if (currentCamera == IR_CAMERA_IO && irCamera != null) {
            irCamera!!.setPreviewCallback { data, camera ->

                Log.d(this.javaClass.canonicalName, "ir camera data size = ${data.size}")
                rsRgbInAllocation.copyFrom(data)
                yuvToRgbIntrinsic.forEach(rsRgbOutAllocation)
                rsRgbOutAllocation.copyTo(previewIrFrame)

                irFrame.put(previewIrFrame)
//                saveFrame(irFrame, currentCamera)
                camera.setPreviewCallback(null)
//                irFrame.clear()
            }
        }
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
                ) {
                    if (currentCamera == RGB_CAMERA_ID) {
                        startRgb()
                    } else {
                        startIr()
                    }

//                    autoPause()
//                    pushFromFiles()
                } else {
                }
                return
            }
        }
    }

    fun startRgb() {
        rgbCamera = getCameraInstance(RGB_CAMERA_ID)!!
        configureCamera(rgbCamera!!)
//        configureRgbCameraPreview()
        rgbCamera!!.setPreviewDisplay(camera_surface.holder)
        rgbCamera!!.setDisplayOrientation(270)
        rgbCameraDataCallback()
        rgbCamera!!.startPreview()
    }

    private fun startIr() {
        irCamera = getCameraInstance(IR_CAMERA_IO)!!
        configureCamera(irCamera!!)
        irCamera!!.setPreviewDisplay(camera_surface.holder)
        irCamera!!.setDisplayOrientation(270)
//        startIrCameraPreview()
//                    irCamera!!.setPreviewTexture(fakeSurfaceTexture)
        irCameraDataCallback()
        irCamera!!.startPreview()
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
//            rsIrInAllocation.copyFrom(data)
//            yuvToIrIntrinsic.forEach(rsIrOutAllocation)
//            rsIrOutAllocation.copyTo(previewIrFrame)

//            irFrameSemaphor = true
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
            surface: SurfaceTexture?,
            width: Int,
            height: Int
        ) {
        }

        override fun onSurfaceTextureUpdated(surface: SurfaceTexture?) {
        }

        override fun onSurfaceTextureDestroyed(surface: SurfaceTexture?): Boolean {
            return true
        }

        override fun onSurfaceTextureAvailable(surface: SurfaceTexture?, width: Int, height: Int) {
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

        rsRgbInAllocation.copyFrom(data)
        yuvToRgbIntrinsic.forEach(rsRgbOutAllocation)
        rsRgbOutAllocation.copyTo(previewRGBFrame)

        rgbFrame.clear()
        rgbFrame.put(previewRGBFrame)
        if (!irFrameCopyDataSemaphor) {
//            pushFrame(rgbFrame, irFrame, System.currentTimeMillis())

//            thread(start = true) {
//                pushByteBuffer(previewRGBFrame, previewIrFrame, System.currentTimeMillis())
//            }
            irFrameCopyDataSemaphor = true
        }

        camera?.addCallbackBuffer(bestShotCallbackBuffer)
    }


    fun pause() {
        if (currentCamera == RGB_CAMERA_ID && rgbCamera != null) {
            rgbCamera!!.setPreviewCallback(null)
            rgbCamera!!.stopPreview()
            rgbCamera!!.release()
            rgbCamera = null
        }

        if (currentCamera == IR_CAMERA_IO && irCamera != null) {
            irCamera!!.setPreviewCallback(null)
            irCamera!!.stopPreview()
            irCamera!!.release()
            irCamera = null
        }
    }

    fun resume() {
//        resetCounter()
        if (currentCamera == RGB_CAMERA_ID) {

            rgbCamera = getCameraInstance(RGB_CAMERA_ID)!!
            configureCamera(rgbCamera!!)
            configureRgbCameraPreview()

        } else {
            irCamera = getCameraInstance(IR_CAMERA_IO)!!
            configureCamera(irCamera!!)
            startIrCameraPreview()
            irCameraDataCallback()

        }
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
//        Log.d(TAG, "RETURN CODE IS $initCode")
        return initCode
    }

    fun rsSetup() {
        val rgbArraySize: Int = 640 * 480 * 4

        val nv21Type = Type.Builder(rs, Element.U8(rs))
            .setX(640)
            .setY(480)
            .setYuvFormat(ImageFormat.NV21)
            .create()
        val rgbType = Type.Builder(rs, Element.RGBA_8888(rs))
            .setX(640)
            .setY(480)
            .create()

        yuvToRgbIntrinsic = ScriptIntrinsicYuvToRGB.create(rs, Element.U8_4(rs))
        rsRgbInAllocation = Allocation.createTyped(rs, nv21Type)
        rsRgbOutAllocation = Allocation.createTyped(rs, rgbType)
        previewRGBFrame = ByteArray(rgbArraySize)
        previewRGBFrameRotated = ByteArray(rgbArraySize)
        yuvToRgbIntrinsic.setInput(rsRgbInAllocation)

        yuvToIrIntrinsic = ScriptIntrinsicYuvToRGB.create(rs, Element.U8_4(rs))
        rsIrInAllocation = Allocation.createTyped(rs, nv21Type)
        rsIrOutAllocation = Allocation.createTyped(rs, rgbType)
        previewIrFrame  = ByteArray(rgbArraySize)
        yuvToIrIntrinsic.setInput(rsIrInAllocation)
    }

    private external fun pushFrame(rgbFrame: ByteBuffer, irFrame: ByteBuffer, frameTimestamp: Long )
    private external fun pushFromFiles()
    //    private external fun uffer(rgbFrame: ByteArray , irFrame: ByteArray, frameTimestamp: Long )
    private external fun initFaceEngine(
        path: String?,
        livenessThreshold: Float,
        yawThreshold: Float,
        pitchThreshold: Float,
        rollThreshold: Float
    ): Int
    private external fun resetCounter()
    private external fun startDetecting()
    private external fun stopDetecting()
    private external fun saveFrame(rgbFrame: ByteBuffer, currentCamera: Int)
}