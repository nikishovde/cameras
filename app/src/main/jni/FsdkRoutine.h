#include <vector>
#include <string>
#include <thread>
#include <stdarg.h>

#include <fsdk/FaceEngine.h>
#include <fsdk/IRefCounted.h>
#include <fsdk/Types/ResultValue.h>
#include <trackEngine/ITrackEngine.h>
#include <stdio.h>
#include <time.h>   // clock_gettime, CLOCK_REALTIME
#include <unistd.h> // sleep
#include <unordered_map>

#include <android/log.h>

#include <jni.h>

#define LOG_INFO(TAG, ...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__);
#define LOG_WARN(TAG, ...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__);
#define LOG_FATAL(TAG, ...) __android_log_print(ANDROID_LOG_FATAL, TAG, __VA_ARGS__);

#include"Timer.h"

using namespace fsdk;
using namespace tsdk;
using namespace std::chrono;
using ITrackEnginePtr = fsdk::Ref<tsdk::ITrackEngine>;
using IStreamPtr = fsdk::Ref<tsdk::IStream>;

static IFaceEnginePtr engine = nullptr;
static ITrackEnginePtr t_engine = nullptr;
static IDetectorPtr detector = nullptr;
static IDetectorPtr detector_v3 = nullptr;

static IStreamPtr stream = nullptr;

static IQualityEstimatorPtr quality = nullptr;
static IHeadPoseEstimatorPtr headpose = nullptr;
static ILivenessIRRGBEstimatorPtr livenessIRRGBEstimatorPtr = nullptr;

static IWarperPtr warper = nullptr;

float liveness_threshold;
float yaw_threshold;
float pitch_threshold;
float roll_threshold;

static uint32_t frameCounter = {};

static bool save_raw_photos, save_bestshot_photos = false;
static int live_frames_count = 1;

struct FrameData: virtual tsdk::AdditionalFrameData {
    fsdk::Image irImage;
};
// face detection limits properties
static int dead_zone_width;
static int preview_container_padding = 15;
// measured for 640x480 resolution to prevent warp image truncation
static int effective_bounding_box_height = 340;
static int effective_bounding_box_width = 440;

//// processing callback to handler class
//typedef struct PhotoProcessodContext {
//    JavaVM   *javaVM;
//    jclass    frameObserverClass;
//    jclass    jniLiveFrame;
//    jobject   frameObserverObj;
//    pthread_mutex_t  lock;
//};
//static PhotoProcessodContext g_ctx;

/*
 * processing one time initialization:
 *     Cache the javaVM into our context
 *     Find class ID for JniHelper
 *     Create an instance of JniHelper
 *     Make global reference since we are using them from a native thread
 * Note:
 *     All resources allocated here are never released by application
 *     we rely on system to free all global refs when it goes away;
 *     the pairing function JNI_OnUnload() never gets called at all.
 */
//JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
//    JNIEnv* env;
//    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
//        return JNI_ERR;
//    }
//    g_ctx.javaVM = vm;
//
//    // Find your class. JNI_OnLoad is called from the correct class loader context for this to work.
//    jclass frameObserver =  env->FindClass("ru/visionlabs/payment/core/FrameObserver");
//    jclass liveFrame     =  env->FindClass("ru/visionlabs/payment/photo/presentation/LiveFrame");
//
//    if (frameObserver == nullptr) {
//        LOG_FATAL("FsdkRoutine", "Failed to ru/visionlabs/payment/core/PhotoProcessor.java");
//        return JNI_ERR;
//    }
//    g_ctx.frameObserverClass    = static_cast<jclass>(env->NewGlobalRef(frameObserver));
//    g_ctx.jniLiveFrame          = static_cast<jclass>(env->NewGlobalRef(liveFrame));
//
//    jmethodID ctor_observer     = env->GetMethodID(frameObserver, "<init>", "()V");
//    jobject observer            = env->NewObject(frameObserver, ctor_observer);
//
//    g_ctx.frameObserverObj      = static_cast<jobject>(env->NewGlobalRef(observer));
//
//    LOG_INFO("FsdkRoutine", "JNI loaded successfully!");
//    return JNI_VERSION_1_6;
//}


struct TECallbacks : tsdk::IBestShotObserver, tsdk::IBestShotPredicate {

    void bestShot(const tsdk::DetectionDescr &descr, const tsdk::AdditionalFrameData* data) override {
        LOG_INFO("TrackEngineCallbacks", "We have bestshot!!!");
    }

    void trackEnd(const tsdk::TrackId &trackId) override {
        LOG_INFO("TrackEngineCallbacks", "Track Lost!");
    }

    bool checkBestShot(const tsdk::DetectionDescr &descr, const tsdk::AdditionalFrameData* data) override {
        LOG_INFO("TrackEngineCallbacks", "Callback: frame %d", descr.frameIndex);

//        if (live_frames_count <= 0) {
//            LOG_INFO("TrackEngineCallbacks", "Liveframes counting complete.");
//            return false;
//        }
        struct FrameData: tsdk::AdditionalFrameData{
            fsdk::Image irImage;
        };
        auto frameData = static_cast<const FrameData*>(data);
        fsdk::Image irFrame = frameData->irImage;


        LOG_INFO("TrackEngineCallbacks", "IRRGB irFrame: height: %d, width: %d",
                 irFrame.getHeight(), irFrame.getWidth());

//        if (!isDetectionInDisplayLimits(descr)) {
//            return false;
//        }

        if (!isHeadPositionValid(descr)) {
            irFrame.reset();
            return false;
        }

        auto imageWarp = fsdk::Image();
        if (!getImageWarp(descr, imageWarp)) {
            irFrame.reset();
            return false;
        }

        if (!isQualityGood(descr, imageWarp)) {
            irFrame.reset();
            return false;
        }

        fsdk::BaseDetection<float> irDetect;
        if (!getIrDetection(descr, irFrame, irDetect)) {
            irFrame.reset();
            return false;
        }

        float livenessScore;
        if (!isFrameLive(descr, irFrame, irDetect, livenessScore)) {
            irFrame.reset();
            return false;
        }

//        if (--live_frames_count > 0) {
//            /**
//             * skip further processing until live_frames_count
//             */
//            return true;
//        }

//        notifyFrameObserver(imageWarp, livenessScore);
        if (save_bestshot_photos) {
            descr.image.save(
                    ("/sdcard/DCIM/" + std::to_string(livenessScore) +
                     "_score_rgb.png").c_str());
            irFrame.save(("/sdcard/DCIM/" + std::to_string(livenessScore) +
                          "_score_ir.png").c_str());
        }
        irFrame.reset();
        return true;
    }

    bool isDetectionInDisplayLimits(const tsdk::DetectionDescr &descr) {
        /**
         * Check face detection in display limits
         */
        fsdk::Rect limitBox = fsdk::Rect();

        limitBox.x = dead_zone_width - preview_container_padding;
        limitBox.y = dead_zone_width - preview_container_padding * descr.detection.rect.height / effective_bounding_box_height;
        limitBox.width = effective_bounding_box_width;
        limitBox.height = effective_bounding_box_height;

        LOG_INFO("TrackEngineCallbacks", "image sizes: width %d, height %d", descr.image.getWidth(), descr.image.getHeight());

        LOG_INFO("TrackEngineCallbacks", "bounding box left - %d, top - %d, width - %d, height - %d, center - %d;%d",
                 descr.detection.rect.x, descr.detection.rect.y, descr.detection.rect.width, descr.detection.rect.height,
                 descr.detection.rect.center().x, descr.detection.rect.center().y);

        LOG_INFO("TrackEngineCallbacks", "limitBox left - %d, top - %d, right - %d, bottom - %d",
                 limitBox.x, limitBox.y,
                 limitBox.x + limitBox.width, limitBox.y + limitBox.height);

        if (!descr.detection.rect.inside(limitBox)) {
            LOG_WARN("TrackEngineCallbacks", "BoundingBox out of display!");
            return true;
        }
        return true;
    }

    bool isHeadPositionValid(const tsdk::DetectionDescr &descr) {
        /**
         * Head position check
         */
        struct timespec ts1, ts2;
        LOG_INFO("TrackEngineCallbacks", "HEADPOS start check");
        clock_gettime(CLOCK_REALTIME, &ts1);

        fsdk::HeadPoseEstimation hposeOut = {};
        auto headPoseStatus = headpose->estimate(descr.image, descr.detection, hposeOut);
        if (headPoseStatus.isError()) {
            LOG_FATAL("TrackEngineCallbacks", "HEADPOS Failed to estimate by reason : %s", headPoseStatus.what());
            return false;
        }

        clock_gettime(CLOCK_REALTIME, &ts2);
        if (ts2.tv_nsec < ts1.tv_nsec) {
            ts2.tv_nsec += 1000000000;
            ts2.tv_sec--;
        }
        LOG_INFO("TrackEngineCallbacks", "HEADPOS complete in %ld.%09ld ms",
                 (long)(ts2.tv_sec - ts1.tv_sec), ts2.tv_nsec - ts1.tv_nsec);

        if (std::abs(hposeOut.pitch) >= pitch_threshold ||
            std::abs(hposeOut.roll) >= roll_threshold ||
            std::abs(hposeOut.yaw) >= yaw_threshold) {
            LOG_WARN("TrackEngineCallbacks", "HEADPOS angle criteria failed:"
                                             "\n\t\tpitch - %f,\n\t\troll - %f,\n\t\tyaw - %f!",
                     hposeOut.pitch, hposeOut.roll, hposeOut.yaw);
            return false;
        }
        return true;
    }

    bool getImageWarp(const tsdk::DetectionDescr &descr, fsdk::Image &imageWarp) {
        /**
        * Get warp
        */
        struct timespec ts1, ts2;
        LOG_INFO("TrackEngineCallbacks", "WARP start creating transformation");
        clock_gettime(CLOCK_REALTIME, &ts1);
        fsdk::Transformation transformation = warper->createTransformation(descr.detection, descr.landmarks);
        {
            auto warpStatus = warper->warp(descr.image, transformation, imageWarp);
            if (warpStatus.isError()) {
                LOG_FATAL("TrackEngineCallbacks", "WARP Failed to warp incoming frame! Reason : %s",
                          warpStatus.what());
                return false;
            }
        }
        clock_gettime(CLOCK_REALTIME, &ts2);
        if (ts2.tv_nsec < ts1.tv_nsec) {
            ts2.tv_nsec += 1000000000;
            ts2.tv_sec--;
        }
        LOG_INFO("TrackEngineCallbacks", "WARP complete in %ld.%09ld ms",
                 (long)(ts2.tv_sec - ts1.tv_sec), ts2.tv_nsec - ts1.tv_nsec);

        return true;
    }

    bool isQualityGood(const tsdk::DetectionDescr &descr, fsdk::Image &imageWarp) {
        /**
        * Quality estimation
        */
        struct timespec ts1, ts2;
        LOG_INFO("TrackEngineCallbacks", "QUALITY start estimation");
        clock_gettime(CLOCK_REALTIME, &ts1);
        fsdk::SubjectiveQuality qualityOutput = {};
        auto qualityStatus = quality->estimate(imageWarp, qualityOutput);
        if (qualityStatus.isError()) {
            LOG_FATAL("TrackEngineCallbacks", "Failed to estimate quality of incoming frame! reason : %s", qualityStatus.what());
            return false;
        }
        clock_gettime(CLOCK_REALTIME, &ts2);
        if (ts2.tv_nsec < ts1.tv_nsec) {
            ts2.tv_nsec += 1000000000;
            ts2.tv_sec--;
        }
        LOG_INFO("TrackEngineCallbacks", "QUALITY complete in %ld.%09ld ms",
                 (long)(ts2.tv_sec - ts1.tv_sec), ts2.tv_nsec - ts1.tv_nsec);
        if (!qualityOutput.isGood()){

            LOG_FATAL("TrackEngineCallbacks", "QUALITY failed:\n\t\tisBlurred - %d,\n\t\tisHighlighted - %d,"
                                              "\n\t\tisDark - %d,\n\t\tisIlluminated - %d,\n\t\tisNotSpecular - %d",
                    qualityOutput.isBlurred, qualityOutput.isHighlighted, qualityOutput.isDark,
                    qualityOutput.isIlluminated, qualityOutput.isNotSpecular);
            return false;
        }

        return true;
    }

    bool getIrDetection(const tsdk::DetectionDescr &descr, const fsdk::Image &irFrame, fsdk::BaseDetection<float>  &irDetect) {
        /**
        * Get IR detection
        */
        struct timespec ts1, ts2;
        LOG_INFO("TrackEngineCallbacks", "IRRGB get ir detection");
        clock_gettime(CLOCK_REALTIME, &ts1);

        fsdk::Detection irDetection = descr.detection;
        irDetection.rect.adjust(
                -irDetection.rect.x / 2, -irDetection.rect.y / 2,
                irDetection.rect.width, irDetection.rect.height);

        LOG_INFO("TrackEngineCallbacks", "IRRGB adjusted ir detection: x: %d, y: %d, w: %d, h: %d",
                 irDetection.rect.x, irDetection.rect.y, irDetection.rect.width, irDetection.rect.height);

        LOG_INFO("TrackEngineCallbacks", "IRRGB irFrame: height: %d, width: %d",
                 irFrame.getHeight(), irFrame.getWidth());

        fsdk::Image imageIRasRGB;
        irFrame.convert(imageIRasRGB, fsdk::Format::R8G8B8);

        if (!imageIRasRGB.isValid()) {
            LOG_FATAL("TrackEngineCallbacks", "IRRGB fail to convert ir to rgb.");
            return false;
        }
        LOG_INFO("TrackEngineCallbacks", "IR frame converted to RGB");

        fsdk::ResultValue<fsdk::FSDKError, fsdk::Face> irRedetect =
                detector_v3->redetectOne(imageIRasRGB, irDetection.rect);

        clock_gettime(CLOCK_REALTIME, &ts2);
        if (ts2.tv_nsec < ts1.tv_nsec) {
            ts2.tv_nsec += 1000000000;
            ts2.tv_sec--;
        }

        if (irRedetect.isError() || !irRedetect->detection.isValid()) {
            LOG_FATAL("TrackEngineCallbacks", "IRRGB fail to get ir detection: %s, detection status: %d",
                      irRedetect.what(), irRedetect->detection.isValid());
            return false;
        }
        LOG_INFO("TrackEngineCallbacks", "IRRGB get ir detection pass in %ld.%09ld ms. Detection height: %f, isValid - %d",
                 (long)(ts2.tv_sec - ts1.tv_sec),  ts2.tv_nsec - ts1.tv_nsec,
                 irRedetect->detection.rect.height, irRedetect->detection.isValid());
        irDetect = irRedetect->detection;
        LOG_INFO("TrackEngineCallbacks", "bounding box for ir frame left - %d, top - %d, width - %d, height - %d, center - %d;%d",
                 irDetect.rect.x, irDetect.rect.y, irDetect.rect.width,
                 irDetect.rect.height, irDetect.rect.center().x, irDetect.rect.center().y);
        return true;
    }

    bool isFrameLive(const tsdk::DetectionDescr &descr, fsdk::Image &irFrame, fsdk::BaseDetection<float>  &irDetect,
            float &livenessScore) {
        /**
        * Liveness check
        */
        struct timespec ts1, ts2;
        LOG_INFO("TrackEngineCallbacks", "IRRGB start liveness check");
        clock_gettime(CLOCK_REALTIME, &ts1);

        fsdk::IRRGBEstimation irRgbEstimation;
        fsdk::Result<fsdk::FSDKError> livenessIRRGBEstimatorResult =
                livenessIRRGBEstimatorPtr->estimate(
                        descr.image, irFrame,
                        descr.detection, irDetect,
                        irRgbEstimation);

        if (livenessIRRGBEstimatorResult.isError()) {
            LOG_INFO("TrackEngineCallbacks", "IRRGB Face liveness estimation error: %s", livenessIRRGBEstimatorResult.what());
            return false;
        }

        clock_gettime(CLOCK_REALTIME, &ts2);
        if (ts2.tv_nsec < ts1.tv_nsec) {
            ts2.tv_nsec += 1000000000;
            ts2.tv_sec--;
        }

        LOG_INFO("TrackEngineCallbacks", "IRRGB Face liveness estimation pass in %ld.%09ld ms with score %f",
                 (long)(ts2.tv_sec - ts1.tv_sec),  ts2.tv_nsec - ts1.tv_nsec, irRgbEstimation.livenessScore);

        if (irRgbEstimation.livenessScore < liveness_threshold) {
            LOG_FATAL("TrackEngineCallbacks", "IRRGB score is too small");
            return false;
        }
        livenessScore = irRgbEstimation.livenessScore;
        return true;
    }

//    void notifyFrameObserver(fsdk::Image &imageWarp, float livenessScore) {
//        /**
//        * Notify frame observer
//        */
//        struct timespec ts1, ts2;
//        LOG_INFO("TrackEngineCallbacks", "JNI start to notify frame observer");
//        clock_gettime(CLOCK_REALTIME, &ts1);
//
//        JavaVM *javaVM = g_ctx.javaVM;
//        JNIEnv *env;
//        jint res = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
//        if (res != JNI_OK) {
//            res = javaVM->AttachCurrentThread(&env, NULL);
//            if (JNI_OK != res) {
//                LOG_INFO("TrackEngineCallbacks", "JNI Failed to AttachCurrentThread, ErrorCode = %d", res);
//                return;
//            }
//        }
//
//        jmethodID constructor = env->GetMethodID( g_ctx.jniLiveFrame,
//                                                  "<init>",
//                                                  "(F[B)V");
//        // warp image prepare
//        fsdk::Image warpRGBA  = {};
//        auto convertResult = imageWarp.convert(warpRGBA, fsdk::Format::R8G8B8X8);
//        const int bufferSize = warpRGBA.getWidth() * warpRGBA.getHeight() * warpRGBA.getFormat().getByteDepth();
//        jbyteArray outputBuffer = env->NewByteArray(bufferSize);
//        env->SetByteArrayRegion(outputBuffer, 0, bufferSize, static_cast<jbyte*>(warpRGBA.getData()));
////        // bounding box prepare
////        jintArray bBox = env->NewIntArray(4);
////        int boxBuf[4] = {descr.detection.rect.x, descr.detection.rect.y,
////                         descr.detection.rect.width, descr.detection.rect.height};
////        env->SetIntArrayRegion(bBox, 0, 4, boxBuf);
//
//        jobject liveFrame = env->NewObject(
//                g_ctx.jniLiveFrame,
//                constructor,
//                livenessScore, outputBuffer );
//
//        jmethodID onNext = static_cast<jmethodID>(env->GetMethodID(
//                g_ctx.frameObserverClass, "onNext", "(Lru/visionlabs/payment/photo/presentation/LiveFrame;)V"));
//        env->CallVoidMethod(g_ctx.frameObserverObj, onNext, liveFrame);
//        javaVM->DetachCurrentThread();
//
//        clock_gettime(CLOCK_REALTIME, &ts2);
//        if (ts2.tv_nsec < ts1.tv_nsec) {
//            ts2.tv_nsec += 1000000000;
//            ts2.tv_sec--;
//        }
//        LOG_INFO("TrackEngineCallbacks", "JNI frameObserver notified in %ld.%09ld ms!",
//                 (long)(ts2.tv_sec - ts1.tv_sec),  ts2.tv_nsec - ts1.tv_nsec);
//
//    }

} frameCallbacks;


enum INIT_CODE {
    SUCCESS = 0,
    LICENSE_FAILED,
    OBJ_CREATION_FAILED
};

static int initFaceEngine(char const *dataPath,
                          float livenesThreshold,
                          float yawThreshold,
                          float pitchThreshold,
                          float rollThreshold) {
    LOG_INFO("[INIT FACEENGINE]", "Load Face Engine data path: %s \n", dataPath);
    engine = acquire(createFaceEngine(dataPath));

    const std::string licenseConfPath = std::string{dataPath} + "/license.conf";

    if (!engine) {
        LOG_FATAL("[INIT FACEENGINE]", "Failed to create face engine instance.\n");
        return OBJ_CREATION_FAILED;
    }

    fsdk::ILicense *license = engine->getLicense();

    if (!license) {
        LOG_FATAL("[INIT FACEENGINE]", "failed to get license!");
        return LICENSE_FAILED;
    }

    if (!fsdk::activateLicense(license, licenseConfPath.c_str())) {
        LOG_FATAL("[INIT FACEENGINE]", "failed to activate license!");
        return LICENSE_FAILED;
    }

    const std::string trackConfPath = std::string{dataPath} + "/trackengine.conf";
    ISettingsProviderPtr teConfig = fsdk::acquire(fsdk::createSettingsProvider(trackConfPath.c_str()));
    if (!teConfig) {
        LOG_FATAL("[INIT FACEENGINE]", "Failed to create track engine config.: \n");
        return OBJ_CREATION_FAILED;
    }

    t_engine = fsdk::acquire(tsdk::createTrackEngine(engine, teConfig));

    if (!t_engine) {
        LOG_FATAL("[INIT FACEENGINE]", "Failed to create track engine instance.: \n");
        return OBJ_CREATION_FAILED;
    }

    if (!(detector = acquire(engine->createDetector(fsdk::ObjectDetectorClassType::FACE_DET_V2)))) {
        LOG_INFO("[INIT FACEENGINE]", "Failed to create face detector!\n");
        return OBJ_CREATION_FAILED;
    }

    if (!(detector_v3 = acquire(engine->createDetector(fsdk::ObjectDetectorClassType::FACE_DET_V3)))) {
        LOG_INFO("[INIT FACEENGINE]", "Failed to create face detector v3!\n");
        return OBJ_CREATION_FAILED;
    }

    quality = fsdk::acquire(engine->createQualityEstimator());
    if (!quality) {
        LOG_INFO("[INIT FACEENGINE]", "Failed to create quality estimator!\n");
        return OBJ_CREATION_FAILED;
    }

    headpose = fsdk::acquire(engine->createHeadPoseEstimator());
    if (!headpose) {
        LOG_INFO("[INIT FACEENGINE]", "Failed to create headpose estimator!\n");
        return OBJ_CREATION_FAILED;
    }

    livenessIRRGBEstimatorPtr = fsdk::acquire(engine->createIRRGBEstimator());
    if (!livenessIRRGBEstimatorPtr) {
        LOG_INFO("[INIT FACEENGINE]", "Failed to create irRgb estimator!\n");
        return OBJ_CREATION_FAILED;
    }

    warper = fsdk::acquire(engine->createWarper());
    if (!warper) {
        LOG_INFO("[INIT FACEENGINE]", "Failed to create warper!\n");
        return OBJ_CREATION_FAILED;
    }

    stream = fsdk::acquire(t_engine->createStream());
    if (!stream) {
        LOG_INFO("[INIT FACEENGINE]", "Failed to create stream!\n");
        return OBJ_CREATION_FAILED;
    }

    liveness_threshold = livenesThreshold;
    yaw_threshold = yawThreshold;
    pitch_threshold = pitchThreshold;
    roll_threshold = rollThreshold;

    stream->setBestShotObserver(&frameCallbacks);
    stream->setBestShotPredicate(&frameCallbacks);

    LOG_INFO("[INIT FACEENGINE]", " \n", dataPath);

    return SUCCESS;
} // initFaceEngine

static void  setBoundingBoxLimits(jint deadZoneWidth) {

    dead_zone_width = deadZoneWidth;

};
