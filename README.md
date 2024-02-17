# Yet Another Filesystem

This is a simple filesystem for linux to understand VFS(Virtual Filesystem).

# Usage

## Build the environment

Run the ```make env``` to generate the **YAF** environment image.

With this image, you can try **YAF kernel module** and **YAF user-space tools** in the image.

# Design

## Disk layout

```
    ┌──────────┬─────────────┬──────────────────┬─────────┬───────────┐
    │superblock│inode bitmaps│data block bitmaps│inodes   │data blocks│
    ├──────────┼─────────────┼──────────────────┼─────────┼───────────┤
    │          │             │                  │         │           │
    ▼          ▼             ▼                  ▼         ▼           ▼
 BID_MIN   BID_SB_MAX    BID_IB_MAX       BID_DBB_MAX BID_I_MAX   BID_DB_MAX
BID_SB_MIN BID_IB_MIN    BID_DBB_MIN       BID_I_MIN  BID_DB_MIN   BID_MAX
```

# Reference 

1. [psankar/simplefs](https://github.com/psankar/simplefs)
2. [Toy-VFS](https://github.com/gishsteven/Toy-VFS)
3. [sysprog21/simplefs](https://github.com/sysprog21/simplefs)
4. [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/Chinese/40.pdf)
