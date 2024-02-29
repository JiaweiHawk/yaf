#ifndef __SUPER_H_

    #define __SUPER_H_

    #define KiB * (1024)
    #define YAF_BLOCK_SIZE (4 KiB)

    /*
     * superblock layout
     *
     * yaf_sb_info               on-disk superblock
     * ┌──────┐       ┌──────┬────────────────────────────────┐◄──0  bytes
     * │nr_ibp◄───────►nr_ibp│number of inode bitmap blocks   │
     * ├──────┐       ┌──────┼────────────────────────────────┤◄──4  bytes
     * │nr_dbp◄───────►nr_dbp│number of data bitmap blocks    │
     * ├──────┐       ┌──────┼────────────────────────────────┤◄──8  bytes
     * │nr_i  ◄───────►nr_i  │number of inode blocks          │
     * ├──────┐       ┌──────┼────────────────────────────────┤◄──12 bytes
     * │nr_d  ◄───────►nr_d  │number of data blocks           │
     * └──────┘       ┌──────┼────────────────────────────────┤◄──16 bytes
     *                │      │                                │
     *                │magic │fill with the magic string "yaf"│
     *                │      │                                │
     *                └──────┴────────────────────────────────┘
     */
    #define MAGIC "yaf"

    #ifdef __KERNEL__
        #include <linux/types.h>
    #else // __KERNEL__
        #include <stdint.h>
    #endif // __KERNEL__
    typedef struct YAF_SB_INFO {
        uint32_t nr_ibp; /*number of inode bitmap blocks*/
        uint32_t nr_dbp; /*number of data bitmap blocks*/
        uint32_t nr_i;   /*number of inode blocks*/
        uint32_t nr_d;   /*number of data blocks*/
    } Yaf_Sb_Info;

    /* on-disk superblock structure */
    typedef struct YAF_SUPERBLOCK {
        Yaf_Sb_Info yaf_sb_info;
        char magic[YAF_BLOCK_SIZE - sizeof(Yaf_Sb_Info)];
    } Yaf_Superblock;

    #ifndef __KERNEL__
        #include <assert.h>
    #endif // __KERNEL__
    static_assert(sizeof(Yaf_Superblock) == YAF_BLOCK_SIZE);

    #ifdef __KERNEL__
        #include <linux/fs.h>
        /* fill the in-memory superblock according to on-disk superblock */
        int yaf_fill_super(struct super_block *sb, void *data, int flags);

        /* initialize the *Yaf_Sb_Info* cache */
        long yaf_init_inode_cache(void);

        /* finalize the *Yaf_Sb_Info* cache */
        void yaf_fini_inode_cache(void);
    #endif // __KERNEL__

#endif // __SUPER_H_
