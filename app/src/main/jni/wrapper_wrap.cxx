
#include <jni.h>
#include "FsdkRoutine.h"
#include <chrono>
#include <thread>

bool isPushing = true;
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL Java_ru_visionlab_payment_MainActivity_initFaceEngine(
        JNIEnv *jenv,
        jobject jcls,
        jstring path,
        jfloat livenessThreshold,
        jfloat yawThreshold,
        jfloat pitchThreshold,
        jfloat rollThreshold )
{
    const char *nativeString = (char *) jenv->GetStringUTFChars(path, 0);
    int result = initFaceEngine(
            nativeString,
            livenessThreshold,
            yawThreshold,
            pitchThreshold,
            rollThreshold);
    jenv->ReleaseStringUTFChars(path, nativeString);
    return result;
}

JNIEXPORT void JNICALL Java_ru_visionlab_payment_FaceEngine_setSessionParams(
        JNIEnv *env, jclass clazz,
        jboolean saveRawPhotos,
        jboolean saveBestshotPhotos,
        jint liveFramesCount) {

    save_raw_photos = saveRawPhotos;
    save_bestshot_photos = saveBestshotPhotos;
    live_frames_count = liveFramesCount;
}

JNIEXPORT void JNICALL Java_ru_visionlab_payment_MainActivity_pushFrame(
        JNIEnv *env, jobject thiz, jobject rgb_frame,
        jobject ir_frame, jlong frame_timestamp) {


//    if (live_frames_count <= 0) {
//        LOG_INFO("TrackEngineCallbacks", "Frames processing completed.");
//        return;
//    }

    struct timespec ts1, ts2;

    /**
     * get rgb frame as fsdk::Image
     */
    fsdk::Image imageRGB(640, 480, fsdk::Format::R8G8B8X8, env->GetDirectBufferAddress(rgb_frame));
    fsdk::Image image_rgb;
    imageRGB.convert(image_rgb, fsdk::Format::R8G8B8);
    /**
    * get ir frame as fsdk::Image
    */
    fsdk::Image imageIR(640, 480, fsdk::Format::R8G8B8X8, env->GetDirectBufferAddress(ir_frame));
    fsdk::Image image_ir;
    imageIR.convert(image_ir, fsdk::Format::R8);
    LOG_INFO("TrackEngine","Ir frame size - %d ", image_ir.getDataSize());

    LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
    auto frameData = new FrameData();
    /**
     * setup callback data
     */
    frameData->irImage = image_ir;

    bool isFrameAppended = stream->pushFrame(image_rgb, frameCounter, frameData);
//    bool isFrameAppended = false;
    if(!isFrameAppended)
    {
        LOG_WARN("TrackEngine", "Frame  %d was skipped! Image queue is full!", frameCounter);
    } //else {
    //  ir_frames.insert(IrFrames::value_type(frameCounter, imageIR));
    // }
    frameCounter++;

    if (save_raw_photos && isFrameAppended) {
        LOG_INFO("TrackEngine", "Frame height - %d, width - %d, size - %d",
                 imageRGB.getHeight(), imageRGB.getWidth(), imageRGB.getDataSize());
        image_rgb.save(("/sdcard/DCIM/" + std::to_string(frame_timestamp) +
                        "_ms_rgb.png").c_str());
        image_ir.save(("/sdcard/DCIM/" + std::to_string(frame_timestamp) +
                       "_ms_ir.png").c_str());
    }
}

JNIEXPORT void JNICALL Java_ru_visionlab_payment_MainActivity_pushFromFiles(
        JNIEnv *env, jobject thiz) {


    struct timespec ts1, ts2;

    /**
     * get rgb frame as fsdk::Image
     */
    fsdk::Image image1_rgb, image2_rgb, image3_rgb, image4_rgb, image5_rgb, image6_rgb, image7_rgb, image8_rgb, image9_rgb;
    fsdk::Image image1_ir, image2_ir, image3_ir, image4_ir, image5_ir, image6_ir, image7_ir, image8_ir, image9_ir;
    image1_rgb.load("/sdcard/DCIM/1598260925170_ms_rgb.png", fsdk::Format::R8G8B8);
    image2_rgb.load("/sdcard/DCIM/1598260925368_ms_rgb.png", fsdk::Format::R8G8B8);
    image3_rgb.load("/sdcard/DCIM/1598260925569_ms_rgb.png", fsdk::Format::R8G8B8);
    image4_rgb.load("/sdcard/DCIM/1598260925770_ms_rgb.png", fsdk::Format::R8G8B8);
    image5_rgb.load("/sdcard/DCIM/1598260926001_ms_rgb.png", fsdk::Format::R8G8B8);
    image6_rgb.load("/sdcard/DCIM/1598260926404_ms_rgb.png", fsdk::Format::R8G8B8);
    image7_rgb.load("/sdcard/DCIM/1598269653011_ms_rgb.png", fsdk::Format::R8G8B8);
    image8_rgb.load("/sdcard/DCIM/1598269653209_ms_rgb.png", fsdk::Format::R8G8B8);
    image9_rgb.load("/sdcard/DCIM/1598269654276_ms_rgb.png", fsdk::Format::R8G8B8);


    image1_ir.load("/sdcard/DCIM/1598260925368_ms_ir.png", fsdk::Format::R8);
    image2_ir.load("/sdcard/DCIM/1598260925569_ms_ir.png", fsdk::Format::R8);
    image3_ir.load("/sdcard/DCIM/1598260925770_ms_ir.png", fsdk::Format::R8);
    image4_ir.load("/sdcard/DCIM/1598260926001_ms_ir.png", fsdk::Format::R8);
    image5_ir.load("/sdcard/DCIM/1598260926201_ms_ir.png", fsdk::Format::R8);
    image6_ir.load("/sdcard/DCIM/1598260926404_ms_ir.png", fsdk::Format::R8);
    image7_ir.load("/sdcard/DCIM/1598269653209_ms_ir.png", fsdk::Format::R8);
    image8_ir.load("/sdcard/DCIM/1598269653418_ms_ir.png", fsdk::Format::R8);
    image9_ir.load("/sdcard/DCIM/1598269654474_ms_ir.png", fsdk::Format::R8);

//    LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
//    auto frameData = new FrameData();

    int i = 1;
    bool isFrameAppended;
    while(true) {
        auto frameData = new FrameData();
        LOG_INFO("TrackEngine","iterating %i", i % 10);
        switch (i % 10)
        {
            case 1:
                LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
                frameData->irImage = image1_ir;
                isFrameAppended = stream->pushFrame(image1_rgb, frameCounter, frameData);
                frameCounter++;
                break;
            case 2:
                 LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
                frameData->irImage = image2_ir;
                isFrameAppended = stream->pushFrame(image2_rgb, frameCounter, frameData);
                frameCounter++;
                break;
            case 3:
                LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
                frameData->irImage = image3_ir;
                isFrameAppended = stream->pushFrame(image3_rgb, frameCounter, frameData);
                frameCounter++;
                break;
            case 4:
                LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
                frameData->irImage = image4_ir;
                isFrameAppended = stream->pushFrame(image4_rgb, frameCounter, frameData);
                frameCounter++;
                break;
            case 5:
                LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
                frameData->irImage = image5_ir;
                isFrameAppended = stream->pushFrame(image5_rgb, frameCounter, frameData);
                frameCounter++;
                break;
            case 6:
                LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
                frameData->irImage = image6_ir;
                isFrameAppended = stream->pushFrame(image6_rgb, frameCounter, frameData);
                frameCounter++;
                break;
            case 7:
                LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
                frameData->irImage = image7_ir;
                isFrameAppended = stream->pushFrame(image7_rgb, frameCounter, frameData);
                frameCounter++;
                break;
            case 8:
                LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
                frameData->irImage = image8_ir;
                isFrameAppended = stream->pushFrame(image8_rgb, frameCounter, frameData);
                frameCounter++;
                break;
            case 9:
                LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
                frameData->irImage = image9_ir;
                isFrameAppended = stream->pushFrame(image9_rgb, frameCounter, frameData);
                frameCounter++;
                break;
            default:
                delete frameData;
                break;
        }

        if(!isFrameAppended)
        {
            LOG_WARN("TrackEngine", "Frame  %d was skipped! Image queue is full!", frameCounter);
        }
        i++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    };
}

JNIEXPORT void JNICALL
Java_ru_visionlab_payment_MainActivity_pushByteBuffer(
        JNIEnv *env, jobject thiz,
        jbyteArray rgb_frame, jbyteArray ir_frame,
        jlong frame_timestamp) {

    if (isPushing) {
        LOG_INFO("TrackEngineCallbacks", "Frames processing completed.");
        return;
    }

    /**
     * rgb frame
     */
    std::vector<uint8_t> rgbBuff;
    rgbBuff.resize(env->GetArrayLength(rgb_frame));
    env->GetByteArrayRegion(rgb_frame, 0, rgbBuff.size(), reinterpret_cast<jbyte*>(rgbBuff.data()));

    fsdk::Image imageRGB(640, 480, fsdk::Format::R8G8B8, rgbBuff.data());

    /**
     * ir frame
     */
    std::vector<uint8_t> irBuff;
    irBuff.resize(env->GetArrayLength(ir_frame));
    env->GetByteArrayRegion(ir_frame, 0, irBuff.size(), reinterpret_cast<jbyte*>(irBuff.data()));

    fsdk::Image imageIR(640, 480, fsdk::Format::R8G8B8, irBuff.data());
    fsdk::Image imageIR_R8;
    imageIR.convert(imageIR_R8, fsdk::Format::R8);
    LOG_INFO("TrackEngine","Ir frame size - %d ", imageIR_R8.getDataSize());

    LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
    bool isFrameAppended = stream->pushFrame(imageRGB, frameCounter, nullptr);
//    bool isFrameAppended = false;
    if(!isFrameAppended)
    {
        LOG_WARN("TrackEngine", "Frame  %d was skipped! Image queue is full!", frameCounter);
    } //else {
    //  ir_frames.insert(IrFrames::value_type(frameCounter, imageIR));
    // }
    frameCounter++;
    if (save_raw_photos && isFrameAppended) {
        LOG_INFO("TrackEngine", "Frame height - %d, width - %d, size - %d",
                 imageRGB.getHeight(), imageRGB.getWidth(), imageRGB.getDataSize());
        imageRGB.save(("/sdcard/DCIM/" + std::to_string(frame_timestamp) +
                       "_ms_rgb.png").c_str());
        imageIR.save(("/sdcard/DCIM/" + std::to_string(frame_timestamp) +
                      "_ms_ir.png").c_str());
    }
}

JNIEXPORT void JNICALL
Java_ru_visionlab_payment_MainActivity_startDetecting(JNIEnv *env, jobject thiz) {
    fsdk::Image image_rgb;
    image_rgb.load("/sdcard/DCIM/detection_rgb.png", fsdk::Format::R8G8B8);
    fsdk::Image image_ir;
    image_ir.load("/sdcard/DCIM/detection_ir.png", fsdk::Format::R8);

    isPushing = true;
    bool isFrameAppended;
    while(isPushing) {
        auto frameData = new FrameData();
        LOG_INFO("TrackEngine","Pushing frame %d ", frameCounter);
        frameData->irImage = image_ir;
        isFrameAppended = stream->pushFrame(image_rgb, frameCounter, frameData);
        frameCounter++;
        if(!isFrameAppended)
        {
            LOG_WARN("TrackEngine", "Frame  %d was skipped! Image queue is full!", frameCounter);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

JNIEXPORT void JNICALL
Java_ru_visionlab_payment_MainActivity_stopDetecting(JNIEnv *env, jobject thiz) {
    isPushing = false;
}

JNIEXPORT void JNICALL
Java_ru_visionlab_payment_MainActivity_saveFrame(JNIEnv *env, jobject thiz, jobject frame_data,
                                                 jint current_camera) {

    LOG_INFO("TrackEngine","Saving for %i camera", current_camera);
    if (current_camera == 0) {
        fsdk::Image imageRGB( 640, 480, fsdk::Format::R8G8B8X8,
                             env->GetDirectBufferAddress(frame_data));
        fsdk::Image image_rgb;
        imageRGB.convert(image_rgb, fsdk::Format::R8G8B8);

        image_rgb.save("/sdcard/DCIM/detection_rgb.png");
        LOG_INFO("TrackEngine","Rgb Camera. Saved image with size %d", image_rgb.getDataSize());
    } else {
        fsdk::Image imageIR( 640, 480, fsdk::Format::R8G8B8X8, env->GetDirectBufferAddress(frame_data));
        fsdk::Image image_ir;
        imageIR.convert(image_ir, fsdk::Format::R8);
        image_ir.save("/sdcard/DCIM/detection_ir.png");
        LOG_INFO("TrackEngine","Ir Camera. Saved image with size %d", image_ir.getDataSize());
    }
}

JNIEXPORT void JNICALL
Java_ru_visionlab_payment_MainActivity_resetCounter(JNIEnv *env, jobject thiz) {
    live_frames_count = 10;
}

JNIEXPORT void JNICALL
Java_ru_visionlab_payment_MainActivity_stopCPushing(JNIEnv *env, jobject thiz) {
    isPushing = false;
}

JNIEXPORT void JNICALL Java_ru_visionlabs_payment_javabindings_TrackEngine_setBoundingBoxLimits(
        JNIEnv *env,
        jclass clazz,
        jint deadZoneWidth) {

    setBoundingBoxLimits(deadZoneWidth);
}

#ifdef __cplusplus
}
#endif