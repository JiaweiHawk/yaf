#ifndef __INODE_H_

    #define __INODE_H_

    /*
     * inode structure
     *
     *                   struct inode                                    on-disk inode
     *                ┌─────────┬───────────┐             ┌──────────┬────────────────────────────────────┐◄──0  bytes
     *                │i_mode   │S_IFREG|RWX│◄───────────►│i_mode    │file mode                           │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────────┤◄──4  bytes
     *                │i_uid    │           │◄───────────►│i_uid     │owner id                            │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────────┤◄──8  bytes
     *                │i_gid    │           │◄───────────►│i_gid     │group id                            │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────────┤◄──12 bytes
     *                │i_nlink  │           │◄───────────►│i_nlink   │number of hard links                │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────────┤◄──16 bytes
     *                │__i_atime│           │◄───────────►│i_atime   │inode access time                   │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────────┤◄──20 bytes
     *                │__i_mtime│           │◄───────────►│i_mtime   │inode modification time             │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────────┤◄──24 bytes
     *                │__i_ctime│           │◄───────────►│i_ctime   │inode change time                   │
     *                ├─────────┼───────────┤             ├──────────┼────────────────────────────────────┤◄──28 bytes
     *                │i_size   │           │◄───────────►│i_size    │inode data size in bytes            │
     * ┌──────────┬──┐└─────────┴───────────┘             ├──────────┼────────────────────────────────────┤◄──32 bytes     ┌──────────┐
     * │i_block[0]│  │◄──────────────────────────────────►│i_block[0]│block id for the data block         │◄──────────────►│data block│
     * ├──────────┼──┤                                    ├──────────┼────────────────────────────────────┤◄──36 bytes     └──────────┘  ┌──────────┐
     * │i_block[1]│  │◄──────────────────────────────────►│i_block[1]│block id for the data block         │◄────────────────────────────►│data block│
     * ├──────────┼──┤                                    ├──────────┼────────────────────────────────────┤◄──40 bytes     ┌──────────┐  └──────────┘
     * │ ........ │  │◄──────────────────────────────────►│ ........ │                                    │◄──────────────►│data block│
     * ├──────────┼──┤                                    ├──────────┼────────────────────────────────────┤◄──56 bytes     └──────────┘   ┌──────────┐
     * │i_block[6]│  │◄──────────────────────────────────►│i_block[6]│block id for the data block         │◄─────────────────────────────►│data block│
     * ├──────────┼──┤                                    ├──────────┼────────────────────────────────────┤◄──60 bytes                    └──────────┘
     * │i_block[7]│  │◄──────────────────────────────────►│i_block[7]│block id for the indirect data block│
     * └──────────┴──┘                                    └──────────┴─────────────┬──────────────────────┘◄──64 bytes
     * struct yaf_inode_info                                                       │
     *                                                                             │
     *                                                                             │
     *                                                                             │
     *                                                                             ▼
     *                                                                      indirect data block
     *                                                     ┌─────────────┬───────────────────────────┐◄──0    bytes  ┌──────────┐
     *                                                     │i_block[0]   │block id for the data block│◄─────────────►│data block│
     *                                                     ├─────────────┼───────────────────────────┤◄──4    bytes  └──────────┘
     *                                                     │ ..........  │                           │
     *                                                     ├─────────────┼───────────────────────────┤◄──4092 bytes  ┌──────────┐
     *                                                     │i_block[1023]│block id for the data block│◄─────────────►│data block│
     *                                                     └─────────────┴───────────────────────────┘◄──4096 bytes  └──────────┘
     *
     *
     *                     struct inode                                     on-disk inode
     *                  ┌─────────┬───────────┐             ┌──────────┬─────────────────────────────┐◄──0  bytes
     *                  │i_mode   │S_IFDIR|RWX│◄───────────►│i_mode    │file mode                    │
     *                  ├─────────┼───────────┤             ├──────────┼─────────────────────────────┤◄──4  bytes
     *                  │i_uid    │           │◄───────────►│i_uid     │owner id                     │
     *                  ├─────────┼───────────┤             ├──────────┼─────────────────────────────┤◄──8  bytes
     *                  │i_gid    │           │◄───────────►│i_gid     │group id                     │
     *                  ├─────────┼───────────┤             ├──────────┼─────────────────────────────┤◄──12 bytes
     *                  │i_nlink  │           │◄───────────►│i_nlink   │number of hard links         │
     *                  ├─────────┼───────────┤             ├──────────┼─────────────────────────────┤◄──16 bytes
     *                  │__i_atime│           │◄───────────►│i_atime   │inode access time            │
     *                  ├─────────┼───────────┤             ├──────────┼─────────────────────────────┤◄──20 bytes
     *                  │__i_mtime│           │◄───────────►│i_mtime   │inode modification time      │
     *                  ├─────────┼───────────┤             ├──────────┼─────────────────────────────┤◄──24 bytes
     *                  │__i_ctime│           │◄───────────►│i_ctime   │inode change time            │
     *                  ├─────────┼───────────┤             ├──────────┼─────────────────────────────┤◄──28 bytes
     *                  │i_size   │           │◄───────────►│i_size    │inode data size in bytes     │
     * ┌──────────┬──┐  └─────────┴───────────┘             ├──────────┼─────────────────────────────┤◄──32 bytes
     * │i_block[0]│  │◄────────────────────────────────────►│i_block[0]│block id for the dentry block│
     * ├──────────┼──┤                                      ├──────────┼─────────────────────────────┤◄──36 bytes
     * │i_block[1]│  │◄────────────────────────────────────►│i_block[1]│block id for the dentry block│
     * ├──────────┼──┤                                      ├──────────┼─────────────────────────────┤◄──40 bytes
     * │ ........ │  │◄────────────────────────────────────►│ .......  │                             │
     * ├──────────┼──┤                                      ├──────────┼─────────────────────────────┤◄──56 bytes
     * │i_block[6]│  │◄────────────────────────────────────►│i_block[6]│block id for the dentry block│
     * ├──────────┼──┤                                      ├──────────┼─────────────────────────────┤◄──60 bytes
     * │i_block[7]│  │◄────────────────────────────────────►│i_block[7]│block id for the dentry block│
     * └──────────┴──┘                                      └──────────┴───────────┬─────────────────┘◄──64 bytes
     * struct yaf_inode_info                                                       │
     *                                                                             │
     *                                                                             │
     *                    dentry ◄───────────────────────────────────┐             ▼
     *  ┌──────────┬─────────────────────────┐◄──0    bytes          │       dentry block
     *  │d_ino     │inode id for the dentry  │                       │      ┌───────────┬───────────────┐◄──0    bytes
     *  ├──────────┼─────────────────────────┤◄──4    bytes          └──────┤dentry[0]  │directory entry│
     *  │d_name_len│length of the dentry name│                              ├───────────┼───────────────┤◄──32   bytes
     *  ├──────────┼─────────────────────────┤◄──8    bytes                 │ ........  │               │
     *  │d_name    │dentry name              │                              ├───────────┼───────────────┤◄──4064 bytes
     *  └──────────┴─────────────────────────┘◄──32   bytes                 │dentry[128]│directory entry│
     *                                                                      └───────────┴───────────────┘◄──4096 bytes
     */
    #ifdef __KERNEL__
        #include <linux/types.h>
        #include <linux/fs.h>

        typedef struct YAF_INODE_INFO {
            uint32_t i_block[8];
            struct inode vfs_inode;
        } Yaf_Inode_Info;
    #else // __KERNEL__
        #include <stdint.h>
    #endif // __KERNEL__

    /* on-disk inode structure */
    typedef struct YAF_INODE {
        uint32_t i_mode;            /* file mode */
        uint32_t i_uid;             /* owner id */
        uint32_t i_gid;             /* group id */
        uint32_t i_nlink;           /* number of hard links */
        uint32_t i_size;            /* inode data size in bytes */
        uint32_t i_atime;           /* inode access time */
        uint32_t i_mtime;           /* inode modification time */
        uint32_t i_ctime;           /* inode change time */
        uint32_t i_block[8];        /* block ids for the data block */
    } Yaf_Inode;

    typedef struct YAF_DENTRY {
        uint32_t d_ino;         /* inode id for the dentry */
        uint32_t d_name_len;    /* length of the dentry name */
    } Yaf_Dentry;

    #ifndef __KERNEL__
        #include <assert.h>
    #endif // __KERNEL__
    #include "super.h"
    static_assert(YAF_BLOCK_SIZE % sizeof(Yaf_Inode) == 0);

    /* inode number for the root inode of yaf */
    #define ROOT_INO    0

    /* number of inodes per block */
    #define INODES_PER_BLOCK    (YAF_BLOCK_SIZE / sizeof(Yaf_Inode))

    /* convert inode number to the corresponding block id */
    #include "fs.h"
    #define INO2BID(sb, ino)    (BID_I_MIN((sb)) + \
                                 ((ino)) / YAF_BLOCK_SIZE)

    #define YAF_INODE(inode) \
        ((Yaf_Inode_Info *)container_of(inode, Yaf_Inode_Info, vfs_inode))

    #ifdef __KERNEL__
        #include <linux/fs.h>
        #include <linux/types.h>
        /* fill the in-memory inode according to on-disk inode */
        struct inode *yaf_iget(struct super_block *sb, unsigned long ino);
    #endif // __KERNEL__

    #define YAF_INODE(inode) \
        ((Yaf_Inode_Info *)container_of(inode, Yaf_Inode_Info, vfs_inode))

#endif // __INODE_H_
