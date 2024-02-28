#ifndef __ARGUMENTS_H_

    #define __ARGUMENTS_H_

    typedef struct ARGUMENTS {
        char *device; // path to the device to be used
    } Arguments;

    /* parse arguments from *argv* into *arguments* */
    void mkfs_parse_arguments(Arguments *arguments,
                              int argc, char *argv[]);

#endif // __ARGUMENTS_H_
