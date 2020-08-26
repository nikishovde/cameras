cmake_minimum_required(VERSION 3.6)

list(APPEND CMAKE_MODULE_PATH "${FSDK_EXAMPLES}/cmake/")

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

find_package(FaceEngineSDK REQUIRED)
find_package(TrackEngineSDK REQUIRED)

add_library(wrapper SHARED wrapper_wrap.cxx)

target_include_directories(wrapper PUBLIC
		./
		${FSDK_INCLUDE_DIRS}
		${TSDK_INCLUDE_DIRS}
		)

add_library(faceSdkEngine_prebuilt SHARED IMPORTED)
set_property(TARGET faceSdkEngine_prebuilt PROPERTY IMPORTED_LOCATION "${FSDK_ROOT}/lib/clang/${ANDROID_ABI}/libFaceEngineSDK.so")

add_library(flower_prebuilt SHARED IMPORTED)
set_property(TARGET flower_prebuilt PROPERTY IMPORTED_LOCATION "${FSDK_ROOT}/lib/clang/${ANDROID_ABI}/libflower.so")

add_library(livenessSdkEngine_prebuilt SHARED IMPORTED)
set_property(TARGET livenessSdkEngine_prebuilt PROPERTY IMPORTED_LOCATION "${FSDK_ROOT}/lib/clang/${ANDROID_ABI}/libLivenessEngineSDK.so")

add_library(trackEngine_prebuilt SHARED IMPORTED)
set_property(TARGET trackEngine_prebuilt PROPERTY IMPORTED_LOCATION "${FSDK_ROOT}/lib/clang/${ANDROID_ABI}/libTrackEngineSDK.so")

find_library(loglib log PATH "${ANDROID_NDK}/platforms/${ANDROID_PLATFORM}/arch-${ANDROID_SYSROOT_ABI}/usr/lib/")

target_link_libraries( wrapper PUBLIC
		trackEngine_prebuilt
		faceSdkEngine_prebuilt
		flower_prebuilt
		livenessSdkEngine_prebuilt
		${loglib}
		)
