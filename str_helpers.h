#ifndef ELASTIC_IRC_BOT_STR_HELPERS_H
#define ELASTIC_IRC_BOT_STR_HELPERS_H

#include <stddef.h>

/**
 * Sanitizes strings
 *
 * @param in Input
 * @param out Must be freed!
 * @return Length allocated
 */
size_t sanitize_str(char *in, char **out);

#endif
