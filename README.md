# Yet Another Filesystem

This is a simple filesystem for linux to understand VFS(Virtual Filesystem).

# Usage

## Build the environment

Run the ```make env``` to generate the **YAF** environment image.

With this image, you can try **YAF kernel module** and **YAF user-space tools** in the image.

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
yaf_sb_info    superblock
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
               │magic │fill with the magic string 'yaf'│
               │      │                                │
               └──────┴────────────────────────────────┘
```

# Reference 

1. [psankar/simplefs](https://github.com/psankar/simplefs)
2. [Toy-VFS](https://github.com/gishsteven/Toy-VFS)
3. [sysprog21/simplefs](https://github.com/sysprog21/simplefs)
4. [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/Chinese/40.pdf)
5. [Linux内核—文件系统mount过程](https://zhuanlan.zhihu.com/p/606596107)
