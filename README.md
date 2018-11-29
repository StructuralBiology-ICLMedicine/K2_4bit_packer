# K2 4bit Packer

**Programs**: MRC Gain Estimator and Bit Packing Routines for K2 Data

**Authors**: CHSAylett

**Dated**: 2017/12/19

**Language**: C (GCC and Clang both tested on Mac and GNU/Linux)

**Licence**: GPL

This program is intended to extract, refine and remove the gain reference from MRC format counting data recorded as 32 bit float frames WITHOUT motion correction and, using the extracted gain reference, to pack the original data into 4-bit, mode 101, MRC files. They require the c math library to be linked and POSIX threads.


      Usage - (list_of_mrc_stacks) | k2_bit_packer [ --gain ][ --pack <gain.raw> ] 


MRC stacks in mode 2 (32-bit float) only, are read in from standard input as valid paths ending in ".mrc". Output stacks will be written in the current working directory as "-4bit.mrc".

Option [ --gain ] estimates and refines a gain reference. The initial gain estimate is the minimum over a single frame stack. Refinement of this gain estimate is then by re-estimating the gain value for each pixel in each frame and averaging over a number of stacks.

Option [ --pack <gain.raw> ] packs stacks according to a completed gain reference. Bit packing is according to the non-standard mode 101 MRC format used by several software packages, including motioncor2, and the original image stacks can be recovered by motioncor2 or simple multiplication. It is recommended to try this at least once before archiving data processed this way.

This is NOT, and NEVER will be a recommended procedure - K2 counting data should always be written as integers, with the corresponding gain images retained. This is a workaround to avoid retention of massive amounts of 32bit data with the corresponding overhead on storage and power consumption.

Both programs read a list of image stack filenames on which to operate from a pipe, and require either an output filename for the gain (gain extraction) or two paths (input and output) and an input gain filename (bit packing). The compiled programs describe their own input when called with inappropriate arguments.

The gain extraction routine output a 32bit float gain image in an identical orientation as the first frame of the input stacks, whereas the bit packing routine will output a series of 4bit (MRC 101) files in the output directory specified. If there is a substantial residual on packing, the program will exit to avoid data loss.

Please report bugs, problems, contraindications or issues of any kind to **chsaylett@gmail.com**. These programs are provided without any guarantees of any form. Please note as mentioned above that they DO NOT correspond to a recommended workflow, but instead exist only to rectify the mistakes of the past.
