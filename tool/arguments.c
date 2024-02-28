#include <argp.h>
#include "../include/yaf.h"
#include "arguments.h"

/* available arguments */
static struct argp_option options[] = {
    {},
};

/* parse the arguments */
static error_t parse_opt(int key, char *arg,
                         struct argp_state *state) {
    Arguments *arguments = state->input;
    long ret = 0;

    switch (key) {
        case ARGP_KEY_ARG:
            arguments->device = arg;
            log(LOG_INFO, "parse_opt() sets device to %s", arg);
            break;

        case ARGP_KEY_NO_ARGS:
            log(LOG_ERR, "no device specified");
            argp_usage(state);
            break;

        default:
            ret = ARGP_ERR_UNKNOWN;
            break;
    }

    return ret;
}

static struct argp argp = {
    .options = options,
    .parser = parse_opt,
    .doc = "build a yaf linux filesystem",
    .args_doc = "<device>",
};

/* parse arguments from *argv* into *arguments* */
void mkfs_parse_arguments(Arguments *arguments, int argc, char **argv) {
    argp_parse(&argp, argc, argv, 0, 0, arguments);
    assert(arguments->device);
}
