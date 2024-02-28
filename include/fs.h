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
    #define BID_MIN(sb) 0
    #define BID_SB_MIN(sb) BID_MIN(sb)

#endif // __FS_H_
