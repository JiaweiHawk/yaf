#ifndef __BITMAP_H_

    #define __BITMAP_H_

    /*
     * bitmap structure
     *
     *    least significant bit             most significant bit
     *              │                               │
     *              │                               ▼
     *              ├───┬───┬───┬───┬───┬───┬───┬───┐
     *         Byte │   │   │   │   │   │   │   │   │
     *              └───┴───┴───┴───┴───┴───┴───┴───┘
     *              ▲                               ▲
     *              │                               │
     *              │       ┌───────────────────────┘
     *              │       │
     *              ├───────┼───────┬───────┬───────┬───────┐
     *              │ Byte0 │ Byte1 │ Byte2 │ ..... │ Byten │
     *              └───────┴───────┴───────┴───────┴───────┘
     *
     * ──────────────────────────────────────────────────────────────────►
     *                             bitmap idx growth direction
     */

    /* convert inode number to the  */
    #include "super.h"

    #ifdef __KERNEL__
        #include <linux/bits.h>
    #else // __KERNEL__
        #define BITS_PER_BYTE   (8)
    #endif // __KERNEL
    #define BITS_PER_BLOCK  (YAF_BLOCK_SIZE * BITS_PER_BYTE)

    /* convert bitmap idx to the offset within its corresponding byte */
    static inline uint32_t IDX2BEOFF(uint32_t idx) {
        return idx % BITS_PER_BYTE;
    }

    /* convert bitmap idx to the offset within its corresponding block */
    static inline uint32_t IDX2BKOFF(uint32_t idx) {
        return idx % BITS_PER_BLOCK;
    }

    #include "fs.h"

    /* convert inode bitmap idx to the corresponding block id */
    #define IDXI2BID(sb, idx)   (BID_IBP_MIN(sb) \
                                 + (idx) / BITS_PER_BLOCK)

    /* convert byte offset to the byte mask */
    #define BEOFF2MASK(beoff) \
        ({ \
            uint8_t _beoff = (beoff); \
            assert(_beoff <= 7); \
            ((uint8_t)1 << _beoff); \
        })

    #ifdef __KERNEL__
    #else // __KERNEL__
        /* set the bit at the given *byte offset* in the given *byte* */
        static inline uint8_t yaf_set_bit(uint8_t byte, uint8_t byte_off) {
            assert((byte & BEOFF2MASK(byte_off)) == 0);
            return byte | BEOFF2MASK(byte_off);
        }
    #endif // __KERNEL__

#endif // __BITMAP_H_
