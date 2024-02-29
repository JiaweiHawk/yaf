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
    /* minimum block id */
    static inline unsigned long yaf_bid_min(Yaf_Sb_Info *ysi) {
        return 0;
    }

    /* minimum block id for the superblock section */
    static inline unsigned long yaf_bid_sb_min(Yaf_Sb_Info *ysi) {
        return yaf_bid_min(ysi);
    }
    /* maximum block id for the superblock section */
    static inline unsigned long yaf_bid_sb_max(Yaf_Sb_Info *ysi) {
        return yaf_bid_sb_min(ysi) + 1 - 1;
    }

    /* minimum block id for the inode bitmap section */
    static inline unsigned long yaf_bid_ibp_min(Yaf_Sb_Info *ysi) {
        return yaf_bid_sb_max(ysi) + 1;
    }
    /* maximum block id for the inode bitmap section */
    static inline unsigned long yaf_bid_ibp_max(Yaf_Sb_Info *ysi) {
        return yaf_bid_ibp_min(ysi) + ysi->nr_ibp - 1;
    }

    /* minimum block id for the data bitmap section */
    static inline unsigned long yaf_bid_dbp_min(Yaf_Sb_Info *ysi) {
        return yaf_bid_ibp_max(ysi) + 1;
    }
    /* maximum block id for the data bitmap section */
    static inline unsigned long yaf_bid_dbp_max(Yaf_Sb_Info *ysi) {
        return yaf_bid_dbp_min(ysi) + ysi->nr_dbp - 1;
    }

    /* minimum block id for the inode blocks section */
    static inline unsigned long yaf_bid_i_min(Yaf_Sb_Info *ysi) {
        return yaf_bid_dbp_max(ysi) + 1;
    }
    /* maximum block id for the inode blocks section */
    static inline unsigned long yaf_bid_i_max(Yaf_Sb_Info *ysi) {
        return yaf_bid_i_min(ysi) + ysi->nr_i - 1;
    }

    #ifdef __KERNEL__
        #include <linux/fs.h>

        static inline unsigned long BID_MIN(struct super_block *sb) {
            return yaf_bid_min(YAF_SB(sb));
        }

        static inline unsigned long BID_SB_MIN(struct super_block *sb) {
            return yaf_bid_sb_min(YAF_SB(sb));
        }
        static inline unsigned long BID_SB_MAX(struct super_block *sb) {
            return yaf_bid_sb_max(YAF_SB(sb));
        }

        static inline unsigned long BID_IBP_MIN(struct super_block *sb) {
            return yaf_bid_ibp_min(YAF_SB(sb));
        }
        static inline unsigned long BID_IBP_MAX(struct super_block *sb) {
            return yaf_bid_ibp_max(YAF_SB(sb));
        }

        static inline unsigned long BID_DBP_MIN(struct super_block *sb) {
            return yaf_bid_dbp_min(YAF_SB(sb));
        }
        static inline unsigned long BID_DBP_MAX(struct super_block *sb) {
            return yaf_bid_dbp_max(YAF_SB(sb));
        }

        static inline unsigned long BID_I_MIN(struct super_block *sb) {
            return yaf_bid_i_min(YAF_SB(sb));
        }
        static inline unsigned long BID_I_MAX(struct super_block *sb) {
            return yaf_bid_i_max(YAF_SB(sb));
        }
    #else // __KERNEL__
        static inline unsigned long BID_MIN(Yaf_Superblock *ysb) {
            return yaf_bid_min(&ysb->yaf_sb_info);
        }

        static inline unsigned long BID_SB_MIN(Yaf_Superblock *ysb) {
            return yaf_bid_sb_min(&ysb->yaf_sb_info);
        }
        static inline unsigned long BID_SB_MAX(Yaf_Superblock *ysb) {
            return yaf_bid_sb_max(&ysb->yaf_sb_info);
        }

        static inline unsigned long BID_IBP_MIN(Yaf_Superblock *ysb) {
            return yaf_bid_ibp_min(&ysb->yaf_sb_info);
        }
        static inline unsigned long BID_IBP_MAX(Yaf_Superblock *ysb) {
            return yaf_bid_ibp_max(&ysb->yaf_sb_info);
        }

        static inline unsigned long BID_DBP_MIN(Yaf_Superblock *ysb) {
            return yaf_bid_dbp_min(&ysb->yaf_sb_info);
        }
        static inline unsigned long BID_DBP_MAX(Yaf_Superblock *ysb) {
            return yaf_bid_dbp_max(&ysb->yaf_sb_info);
        }

        static inline unsigned long BID_I_MIN(Yaf_Superblock *ysb) {
            return yaf_bid_i_min(&ysb->yaf_sb_info);
        }
        static inline unsigned long BID_I_MAX(Yaf_Superblock *ysb) {
            return yaf_bid_i_max(&ysb->yaf_sb_info);
        }
    #endif // __KERNEL__

#endif // __FS_H_
