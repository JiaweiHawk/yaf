#include <linux/bitmap.h>
#include <linux/buffer_head.h>
#include "../include/bitmap.h"
#include "../include/inode.h"
#include "asm-generic/bitops/instrumented-atomic.h"

/*
 * Return the unset bit in a given in-memory bitmap memory and
 * atomically mark it.
 *
 * Return *-ENOENT* if bit was not found.
 */
static inline int32_t yaf_get_free_bit(void *addr, uint32_t bits) {
    for (uint32_t nr = 0; nr < bits; ++nr) {
        if (!test_and_set_bit(nr, addr)) {
            return nr;
        }
    }
    return -ENOENT;
}

/* clear the given marked bit */
static inline void yaf_put_bit(void *addr, uint32_t nr) {
    assert(test_and_clear_bit(nr, addr));
}

/*
 * Return an unused inode number and mark it used.
 *
 * Return *RESERVED_INO* if no free inode was found.
 */
uint32_t yaf_get_free_inode(struct super_block *sb) {
    for(uint32_t bid = BID_IBP_MIN(sb); bid <= BID_IBP_MAX(sb); ++bid) {
        uint32_t res;
        struct buffer_head *bh = sb_bread(sb, bid);
        if (!bh) {
            log(LOG_ERR, "sb_bread() failed");
            return RESERVED_INO;
        }

        res = yaf_get_free_bit(bh->b_data, BITS_PER_BLOCK);

        if (res >= 0) {
            mark_buffer_dirty(bh);
            brelse(bh);
            return res;
        }

        brelse(bh);
    }

    return RESERVED_INO;
}

/* mark the given inode as unused */
void yaf_put_inode(struct super_block *sb, uint32_t ino) {
    struct buffer_head *bh = sb_bread(sb, IDXI2BID(sb, ino));
    assert(bh);

    yaf_put_bit(bh->b_data, ino % BITS_PER_BLOCK);

    mark_buffer_dirty(bh);
    brelse(bh);
}

/*
 * Return an unused data block and mark it used.
 *
 * Return *RESERVED_DNO* if no free inode was found.
 */
uint32_t yaf_get_free_dblock(struct super_block *sb) {
    for(uint32_t bid = BID_DBP_MIN(sb); bid <= BID_DBP_MAX(sb); ++bid) {
        uint32_t res;
        struct buffer_head *bh = sb_bread(sb, bid);
        if (!bh) {
            log(LOG_ERR, "sb_bread() failed");
            return RESERVED_INO;
        }

        res = yaf_get_free_bit(bh->b_data, BITS_PER_BLOCK);

        if (res >= 0) {
            brelse(bh);
            mark_buffer_dirty(bh);
            return res;
        }

        brelse(bh);
    }

    return RESERVED_DNO;
}

/* mark the given data block as unused */
void yaf_put_dblock(struct super_block *sb, uint32_t dno) {
    struct buffer_head *bh = sb_bread(sb, IDXD2BID(sb, dno));
    assert(bh);

    yaf_put_bit(bh->b_data, dno % BITS_PER_BLOCK);

    mark_buffer_dirty(bh);
    brelse(bh);
}
