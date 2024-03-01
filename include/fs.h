#ifndef __FS_H_

    #define __FS_H_

    /*
     * partition layout
     *
     *     ┌──────────┬─────────────┬────────────┬────────────┬───────────┐
     *     │superblock│inode bitmap │data bitmap │inode blocks│data blocks│
     *     ├──────────┼─────────────┼────────────┼────────────┼───────────┤
     *     │          │             │            │            │           │
     *     ▼          ▼             ▼            ▼            ▼           ▼
     *  BID_MIN   BID_SB_MAX   BID_IBP_MAX  BID_DBP_MAX   BID_I_MAX   BID_D_MAX
     * BID_SB_MIN BID_IBP_MIN  BID_DBP_MIN   BID_I_MIN    BID_D_MIN    BID_MAX
     */
    #include "super.h"

    #ifdef __KERNEL__
        #include <linux/fs.h>

        /* minimum block id */
        static inline unsigned long BID_MIN(struct super_block *sb) {
            return 0;
        }

        /* minimum block id for the superblock section */
        static inline unsigned long BID_SB_MIN(struct super_block *sb) {
            return BID_MIN(sb);
        }
        /* maximum block id for the superblock section */
        static inline unsigned long BID_SB_MAX(struct super_block *sb) {
            return BID_SB_MIN(sb) + 1 - 1;
        }

        /* minimum block id for the inode bitmap section */
        static inline unsigned long BID_IBP_MIN(struct super_block *sb) {
            return BID_SB_MAX(sb) + 1;
        }
        /* maximum block id for the inode bitmap section */
        static inline unsigned long BID_IBP_MAX(struct super_block *sb) {
            return BID_IBP_MIN(sb) + YAF_SB(sb)->nr_ibp - 1;
        }

        /* minimum block id for the data bitmap section */
        static inline unsigned long BID_DBP_MIN(struct super_block *sb) {
            return BID_IBP_MAX(sb) + 1;
        }
        /* maximum block id for the data bitmap section */
        static inline unsigned long BID_DBP_MAX(struct super_block *sb) {
            return BID_DBP_MIN(sb) + YAF_SB(sb)->nr_dbp - 1;
        }

        /* minimum block id for the inode blocks section */
        static inline unsigned long BID_I_MIN(struct super_block *sb) {
            return BID_DBP_MAX(sb) + 1;
        }
        /* maximum block id for the inode blocks section */
        static inline unsigned long BID_I_MAX(struct super_block *sb) {
            return BID_I_MIN(sb) + YAF_SB(sb)->nr_i - 1;
        }

        /* minimum block id for the data blocks section */
        static inline unsigned long BID_D_MIN(struct super_block *sb) {
            return BID_I_MAX(sb) + 1;
        }
        /* maximum block id for the data blocks section */
        static inline unsigned long BID_D_MAX(struct super_block *sb) {
            return BID_D_MIN(sb) + YAF_SB(sb)->nr_d - 1;
        }
    #else // __KERNEL__
        #include <endian.h>

        /* minimum block id */
        static inline unsigned long BID_MIN(Yaf_Superblock *ysb) {
            return 0;
        }

        /* minimum block id for the superblock section */
        static inline unsigned long BID_SB_MIN(Yaf_Superblock *ysb) {
            return BID_MIN(ysb);
        }
        /* maximum block id for the superblock section */
        static inline unsigned long BID_SB_MAX(Yaf_Superblock *ysb) {
            return BID_SB_MIN(ysb) + 1 - 1;
        }

        /* minimum block id for the inode bitmap section */
        static inline unsigned long BID_IBP_MIN(Yaf_Superblock *ysb) {
            return BID_SB_MAX(ysb) + 1;
        }
        /* maximum block id for the inode bitmap section */
        static inline unsigned long BID_IBP_MAX(Yaf_Superblock *ysb) {
            return BID_IBP_MIN(ysb) + le32toh(ysb->yaf_sb_info.nr_ibp) - 1;
        }

        /* minimum block id for the data bitmap section */
        static inline unsigned long BID_DBP_MIN(Yaf_Superblock *ysb) {
            return BID_IBP_MAX(ysb) + 1;
        }
        /* maximum block id for the data bitmap section */
        static inline unsigned long BID_DBP_MAX(Yaf_Superblock *ysb) {
            return BID_DBP_MIN(ysb) + le32toh(ysb->yaf_sb_info.nr_dbp) - 1;
        }

        /* minimum block id for the inode blocks section */
        static inline unsigned long BID_I_MIN(Yaf_Superblock *ysb) {
            return BID_DBP_MAX(ysb) + 1;
        }
        /* maximum block id for the inode blocks section */
        static inline unsigned long BID_I_MAX(Yaf_Superblock *ysb) {
            return BID_I_MIN(ysb) + le32toh(ysb->yaf_sb_info.nr_i) - 1;
        }

        /* minimum block id for the data blocks section */
        static inline unsigned long BID_D_MIN(Yaf_Superblock *ysb) {
            return BID_I_MAX(ysb) + 1;
        }
        /* maximum block id for the data blocks section */
        static inline unsigned long BID_D_MAX(Yaf_Superblock *ysb) {
            return BID_D_MIN(ysb) + le32toh(ysb->yaf_sb_info.nr_d) - 1;
        }
    #endif // __KERNEL__

#endif // __FS_H_
