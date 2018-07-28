
#ifndef ELASTIC_IRC_BOT_MESSAGE_BUS_H
#define ELASTIC_IRC_BOT_MESSAGE_BUS_H

#include <unistd.h>

struct message_bus;

typedef void (*message_bus_listener_t)(void *data);

struct message_bus *allocate_message_bus(size_t max_listeners);

void deallocate_message_bus(struct message_bus *bus);

void message_bus_bind_listener(struct message_bus *bus,
                               message_bus_listener_t listener);

void message_bus_send_message(struct message_bus *bus, void *data);

#endif
