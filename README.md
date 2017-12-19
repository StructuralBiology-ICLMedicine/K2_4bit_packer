# K2 4bit Packer

**Programs**: MRC Gain Estimator and Bit Packing Routines for K2 Data

**Authors**: CHSAylett

**Dated**: 2017/12/19

**Language**: C (GCC and Clang both tested on Mac and GNU/Linux)

**Licence**: GPL

These programs are intended to extract the gain reference from MRC format counting data < k2_gain_estimator.c > recorded in 16 or 32 bit and, using such an extracted gain reference, to pack the original data into 4-bit, mode 101, MRC files < k2_pack_to_4_bits.c >. They require the c math library to be linked.

Estimation proceeds from several image stacks by calculating the ratio between the current estimate and each new pixel in Z. Bit packing is according to the non-standard mode 101 MRC format used by several software packages, and the original image stacks can be recovered by motioncorr2 or multiplication.

This is NOT, and NEVER will be a recommended procedure - K2 counting data should always be written as integers, with the corresponding gain images retained. This is a workaround to avoid retention of massive amounts of 32bit data with the corresponding overhead on storage and power consumption.

Both programs read a list of image stack filenames on which to operate from a pipe, and require either an output filename for the gain (gain extraction) or two paths (input and output) and an input gain filename (bit packing). The compiled programs describe their own input when called with inappropriate arguments.

The gain extraction routine output a 32bit float gain image in an identical orientation as the first frame of the input stacks, whereas the bit packing routine will output a series of 4bit (MRC 101) files in the output directory specified. If there is a substantial residual on packing, the program will exit to avoid data loss.

Please report bugs, problems, contraindications or issues of any kind to **chsaylett@gmail.com**. These programs are provided without any guarantees of any form. Please note as mentioned above that they DO NOT correspond to a recommended workflow, but instead exist only to rectify the mistakes of the past.