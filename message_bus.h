
#ifndef ELASTIC_IRC_BOT_MESSAGE_BUS_H
#define ELASTIC_IRC_BOT_MESSAGE_BUS_H

#include <stddef.h>

struct message_bus;

struct message_envelope {
  int message_type;
  void *data;
};

typedef void (*message_bus_listener_t)(struct message_envelope *envelope);

struct message_bus *allocate_message_bus(size_t max_listeners);

void deallocate_message_bus(struct message_bus *bus);

void message_bus_bind_listener(struct message_bus *bus,
                               message_bus_listener_t listener);

void message_bus_send_message(struct message_bus *bus, struct message_envelope *envelope);

#endif
