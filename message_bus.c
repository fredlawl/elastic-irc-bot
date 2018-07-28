
#include <stdlib.h>
#include <assert.h>

#include "message_bus.h"

struct message_bus {
  size_t max_listeners;
  size_t listener_count;
  message_bus_listener_t *listeners;
};

struct message_bus *allocate_message_bus(size_t max_listeners) {
  struct message_bus *bus;

  if ((bus = (struct message_bus *) malloc(sizeof(struct message_bus))) == NULL) {
    return NULL;
  }

  bus->listeners = (message_bus_listener_t *) malloc(sizeof(message_bus_listener_t) * max_listeners);
  if (bus->listeners == NULL) {
    assert(bus->listeners != NULL);
    free(bus);
    return NULL;
  }

  bus->max_listeners = max_listeners;
  bus->listener_count = 0;

  return bus;
}

void deallocate_message_bus(struct message_bus *bus) {
  free(bus->listeners);
  free(bus);
}

void message_bus_bind_listener(struct message_bus *bus,
                               message_bus_listener_t func_handler) {
  assert(func_handler != NULL);
  bus->listeners[bus->listener_count] = func_handler;
  bus->listener_count++;
}

void message_bus_send_message(struct message_bus *bus, void *data) {
  for (size_t i = 0; i < bus->listener_count; i++) {
    assert(bus->listeners[i] != NULL);
    bus->listeners[i](data);
  }
}