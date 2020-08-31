package ru.visionlab.payment

import android.content.Context
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.io.InputStream
import java.util.*

object Utils {
    const val PATH_TO_EXTRACTED_VL_DATA = "/vl/data"
    private const val PATH_TO_VL_DATA = "/vl"
    fun createVLDataFolder(filesContext: Context): Boolean {
        // create folder
        val fmd = File(filesContext.filesDir.toString() + PATH_TO_VL_DATA)
        println("CREATE VL DATA FOLDER : " + filesContext.filesDir + PATH_TO_VL_DATA)
        if (!(fmd.mkdirs() || fmd.isDirectory)) {
            println("COULDN'T CREATE VL DATA FOLDER : " + filesContext.filesDir + PATH_TO_VL_DATA)
            return false
        }
        return true
    }

    fun createFilesFromAssetFolder(context: Context, folder: String): Boolean {
        val fmd = File(context.filesDir.toString() + PATH_TO_VL_DATA + "/" + folder)
        if (!(fmd.mkdirs() || fmd.isDirectory)) {
            println("Failed to create folder $folder")
            return false
        }
        try {
            val assetManager = context.assets
            val files = ArrayList(Arrays.asList(*assetManager.list(folder)))
            if (files == null || files.size == 0) {
                println("ERROR: ASSET FILE IS EMPTY!")
                return false
            }
            for (filename in files) {
                val file = File(context.filesDir.toString() + PATH_TO_VL_DATA + "/" + folder + "/" + filename)
                val iStream = assetManager.open("$folder/$filename")
                if (writeBytesToFile(iStream, file)) {
                    println("CREATED FILE FROM ASSETS: " + folder + "/" + filename + "  TO   " + file.absolutePath)
                } else {
                    println("ERROR CREATING FILE FROM ASSETS: " + folder + "/" + filename + "  TO   " + file.absolutePath)
                }
            }
        } catch (ex: IOException) {
            ex.printStackTrace()
            return false
        }
        return true
    }

    @Throws(IOException::class)
    fun writeBytesToFile(`is`: InputStream, file: File?): Boolean {
        var result = false
        var fos: FileOutputStream? = null
        try {
            val data = ByteArray(2048)
            var nbread = 0
            fos = FileOutputStream(file)
            while (`is`.read(data).also { nbread = it } > -1) {
                fos.write(data, 0, nbread)
            }
            result = true
        } catch (e: Exception) {
            e.printStackTrace()
            result = false
        } finally {
            fos?.close()
        }
        return result
    }

    fun rotateClockWise(
        rgba: ByteArray,
        rgbaWidth: Int,
        rgbaHeight: Int,
        rgbOut: ByteArray
    ) {
        for (i in 0 until rgbaWidth) {
            var j = 0
            while (j < rgbaHeight * 3) {
                rgbOut[j + rgbaHeight * 3 * i] =
                    rgba[rgbaWidth * 4 * (rgbaHeight - (j / 3 + 1)) + i * 4]
                rgbOut[j + 1 + rgbaHeight * 3 * i] =
                    rgba[rgbaWidth * 4 * (rgbaHeight - (j / 3 + 1)) + 1 + i * 4]
                rgbOut[j + 2 + rgbaHeight * 3 * i] =
                    rgba[rgbaWidth * 4 * (rgbaHeight - (j / 3 + 1)) + 2 + i * 4]
                j += 3
            }
        }
    }

}