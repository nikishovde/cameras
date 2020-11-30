#pragma version(1)
#pragma rs java_package_name(ru.visionlabs.payment.photo.presentation)
#pragma rs_fp_relaxed

rs_allocation gCurrentFrame;
int inWidth;
int inHeight;

uchar4 RS_KERNEL rotate_270_clockwise(uchar4 in, uint32_t x, uint32_t y) {
    uint32_t inX  = inWidth - 1 - y;
    uint32_t inY = x;

    const uchar4 *out = rsGetElementAt(gCurrentFrame, inX, inY);
    return *out;
}

uchar4 RS_KERNEL rotate_90_counterclockwise (uchar4 in, uint32_t x, uint32_t y) {
    uint32_t inX  = inWidth - 1 - y;
    uint32_t inY = inHeight - 1 - x;

    const uchar4 *out = rsGetElementAt(gCurrentFrame, inX, inY);
    return *out;
}
