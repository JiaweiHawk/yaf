# Yet Another Filesystem

[![github CI](https://github.com/JiaweiHawk/yaf/actions/workflows/main.yml/badge.svg)](https://github.com/JiaweiHawk/yaf/actions/)

This is a simple filesystem for linux to understand VFS(Virtual Filesystem).

# Usage

## build the environment

Run the ```make env``` to generate the **YAF** environment image.

With this image, you can try **YAF kernel module** and **YAF user-space tools** in the image.

## boot the qemu

Run the ```make run``` to boot up the qemu with **yak.ko** in **/mnt/shares** on guest and
a test disk imgage **/dev/vda**

## test the yaf

Run the ```make test``` to run the tests on the yaf environment

## debug the yaf

Run the ```make debug``` to debug the **yaf kernel module** on the yaf environment

# Design

## Partition layout

```
    ┌──────────┬─────────────┬────────────┬────────────┬───────────┐
    │superblock│inode bitmap │data bitmap │inode blocks│data blocks│
    ├──────────┼─────────────┼────────────┼────────────┼───────────┤
    │          │             │            │            │           │
    ▼          ▼             ▼            ▼            ▼           ▼
 BID_MIN   BID_SB_MAX   BID_IBP_MAX  BID_DBP_MAX   BID_I_MAX   BID_D_MAX
BID_SB_MIN BID_IBP_MIN  BID_DBP_MIN   BID_I_MIN    BID_D_MIN    BID_MAX
```

## superblock

The superblock contains the metadata for the partition as below:

```
yaf_sb_info               on-disk superblock
┌──────┐       ┌──────┬────────────────────────────────┐◄──0  bytes
│nr_ibp◄───────►nr_ibp│number of inode bitmap blocks   │
├──────┐       ┌──────┼────────────────────────────────┤◄──4  bytes
│nr_dbp◄───────►nr_dbp│number of data bitmap blocks    │
├──────┐       ┌──────┼────────────────────────────────┤◄──8  bytes
│nr_i  ◄───────►nr_i  │number of inode blocks          │
├──────┐       ┌──────┼────────────────────────────────┤◄──12 bytes
│nr_d  ◄───────►nr_d  │number of data blocks           │
└──────┘       ┌──────┼────────────────────────────────┤◄──16 bytes
               │      │                                │
               │magic │fill with the magic string "yaf"│
               │      │                                │
               └──────┴────────────────────────────────┘
```

## bitmap

Bitmap is used to manage the resource allocation within both the inode blocks and data blocks sections of the disk. Each bit of the bitmap corresponds to the usage status of either an inode or a data block, where *1* denotes occupancy and *0* indicates availability. The structure of the bitmap is shown below:

```
   least significant bit             most significant bit
             │                               │
             │                               ▼
             ├───┬───┬───┬───┬───┬───┬───┬───┐
        Byte │   │   │   │   │   │   │   │   │
             └───┴───┴───┴───┴───┴───┴───┴───┘
             ▲                               ▲
             │                               │
             │       ┌───────────────────────┘
             │       │
             ├───────┼───────┬───────┬───────┬───────┐
             │ Byte0 │ Byte1 │ Byte2 │ ..... │ Byten │
             └───────┴───────┴───────┴───────┴───────┘

──────────────────────────────────────────────────────────────────►
                            bitmap idx growth direction
```

## inode

Inodes are filesystem objects such as regular files, directories, FIFOs and other beasts.

When needed, on-disk inodes are loaded into memory, and modifications to the in-memory inodes are synchronized back to the disk.

For yaf, there are only two types of on-disk inodes: file inodes and directory inodes, their structures are shown as below:

```
                  struct inode                                    on-disk inode
               ┌─────────┬───────────┐             ┌──────────┬────────────────────────────────┐◄──0  bytes
               │i_mode   │S_IFREG|RWX│◄───────────►│i_mode    │file mode                       │
               ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──4  bytes
               │i_uid    │           │◄───────────►│i_uid     │owner id                        │
               ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──8  bytes
               │i_gid    │           │◄───────────►│i_gid     │group id                        │
               ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──12 bytes
               │i_nlink  │           │◄───────────►│i_nlink   │number of hard links            │
               ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──16 bytes
               │__i_atime│           │◄───────────►│i_atime   │inode access time               │
               ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──20 bytes
               │__i_mtime│           │◄───────────►│i_mtime   │inode modification time         │
               ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──24 bytes
               │__i_ctime│           │◄───────────►│i_ctime   │inode change time               │
               ├─────────┼───────────┤             ├──────────┼────────────────────────────────┤◄──28 bytes
               │i_size   │           │◄───────────►│i_size    │inode data size in bytes        │
┌──────────┬──┐└─────────┴───────────┘             ├──────────┼────────────────────────────────┤◄──32 bytes     ┌──────────┐
│i_block[0]│  │◄──────────────────────────────────►│i_block[0]│data block id for the data block│◄──────────────►│data block│
├──────────┼──┤                                    ├──────────┼────────────────────────────────┤◄──36 bytes     └──────────┘  ┌──────────┐
│i_block[1]│  │◄──────────────────────────────────►│i_block[1]│data block id for the data block│◄────────────────────────────►│data block│
├──────────┼──┤                                    ├──────────┼────────────────────────────────┤◄──40 bytes     ┌──────────┐  └──────────┘
│ ........ │  │◄──────────────────────────────────►│ ........ │                                │◄──────────────►│data block│
├──────────┼──┤                                    ├──────────┼────────────────────────────────┤◄──56 bytes     └──────────┘   ┌──────────┐
│i_block[6]│  │◄──────────────────────────────────►│i_block[6]│data block id for the data block│◄─────────────────────────────►│data block│
├──────────┼──┤                                    ├──────────┼────────────────────────────────┤◄──60 bytes                    └──────────┘
│i_block[7]│  │◄──────────────────────────────────►│i_block[7]│data block id for the data block│
└──────────┴──┘                                    └──────────┴────────────────────────────────┘◄──64 bytes
struct yaf_inode_info


                    struct inode                                     on-disk inode
                 ┌─────────┬───────────┐             ┌──────────┬──────────────────────────────────┐◄──0  bytes
                 │i_mode   │S_IFDIR|RWX│◄───────────►│i_mode    │file mode                         │
                 ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──4  bytes
                 │i_uid    │           │◄───────────►│i_uid     │owner id                          │
                 ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──8  bytes
                 │i_gid    │           │◄───────────►│i_gid     │group id                          │
                 ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──12 bytes
                 │i_nlink  │           │◄───────────►│i_nlink   │number of hard links              │
                 ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──16 bytes
                 │__i_atime│           │◄───────────►│i_atime   │inode access time                 │
                 ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──20 bytes
                 │__i_mtime│           │◄───────────►│i_mtime   │inode modification time           │
                 ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──24 bytes
                 │__i_ctime│           │◄───────────►│i_ctime   │inode change time                 │
                 ├─────────┼───────────┤             ├──────────┼──────────────────────────────────┤◄──28 bytes
                 │i_size   │           │◄───────────►│i_size    │inode data size in bytes          │
┌──────────┬──┐  └─────────┴───────────┘             ├──────────┼──────────────────────────────────┤◄──32 bytes
│i_block[0]│  │◄────────────────────────────────────►│i_block[0]│data block id for the dentry block│
├──────────┼──┤                                      ├──────────┼──────────────────────────────────┤◄──36 bytes
│i_block[1]│  │◄────────────────────────────────────►│i_block[1]│data block id for the dentry block│
├──────────┼──┤                                      ├──────────┼──────────────────────────────────┤◄──40 bytes
│ ........ │  │◄────────────────────────────────────►│ .......  │                                  │
├──────────┼──┤                                      ├──────────┼──────────────────────────────────┤◄──56 bytes
│i_block[6]│  │◄────────────────────────────────────►│i_block[6]│data block id for the dentry block|
├──────────┼──┤                                      ├──────────┼──────────────────────────────────┤◄──60 bytes
│i_block[7]│  │◄────────────────────────────────────►│i_block[7]│data block id for the dentry block|
└──────────┴──┘                                      └──────────┴──────────┬───────────────────────┘◄──64 bytes
struct yaf_inode_info                                                      │
                                                                           │
                   dentry ◄──────────────────────────────────┐             ▼
┌──────────┬─────────────────────────┐◄──0    bytes          │       dentry block
│d_ino     │inode id for the dentry  │                       │      ┌───────────┬───────────────┐◄──0    bytes
├──────────┼─────────────────────────┤◄──4    bytes          └──────┤dentry[0]  │directory entry│
│d_name_len│length of the dentry name│                              ├───────────┼───────────────┤◄──32   bytes
├──────────┼─────────────────────────┤◄──8    bytes                 │ ........  │               │
│d_name    │dentry name              │                              ├───────────┼───────────────┤◄──4064 bytes
└──────────┴─────────────────────────┘◄──32   bytes                 │dentry[128]│directory entry│
                                                                    └───────────┴───────────────┘◄──4096 bytes
```

# Reference 

1. [psankar/simplefs](https://github.com/psankar/simplefs)
2. [Toy-VFS](https://github.com/gishsteven/Toy-VFS)
3. [sysprog21/simplefs](https://github.com/sysprog21/simplefs)
4. [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/Chinese/40.pdf)
5. [Linux内核—文件系统mount过程](https://zhuanlan.zhihu.com/p/606596107)
