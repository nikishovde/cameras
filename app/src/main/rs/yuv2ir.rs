#pragma version(1)
#pragma rs java_package_name(ru.visionlabs.payment.photo.presentation)
#pragma rs_fp_relaxed

rs_allocation gCurrentFrame;

uchar RS_KERNEL yuv2rgbFrames(uchar prevPixel, uint32_t x, uint32_t y) {

    uchar curPixel = rsGetElementAtYuv_uchar_Y(gCurrentFrame, x, y);

    int4 rgb;
    rgb.r = curPixel;

    uchar greyscale = clamp(rgb.r, 0, 255);

    return greyscale;
}