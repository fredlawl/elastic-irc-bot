#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread/pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <sts_queue/sts_queue.h>
#include <curl/curl.h>

#include "irc_lexer.h"
#include "irc_token.h"
#include "irc_message_parser.h"
#include "irc_message.h"
#include "message_bus.h"
#include "elastic_search.h"

#define SERVER_IP "38.229.70.22"
#define SERVER_PORT 6667
#define IRC_PASS "PASS secretpass\r\n"
#define IRC_NICK "NICK cezizle\r\n"
#define IRC_CONNECT "USER cezizle 127.0.0.1 chat.freenode.net :Yip yop\r\n"
#define IRC_CHANNEL "JOIN #flawlztest\r\n"

#define MESSAGE_BUS_TYPE_IRC_MSG 1

static int socket_descriptor;
static volatile bool irc_read_thread_stopped = false;

static struct elastic_search* elasticsearch;
static struct elastic_search_connection* elastic_connection;

void write_irc_message_to_elastic_search_listener(struct message_envelope *envelope);
void irc_ping_pong_listener(struct message_envelope *envelope);

struct irc_read_buffer_args {
  StsHeader *queue;
};

void *irc_read_buffer_thread(void *thread_args);
void signal_termination_handler(int signal_num);

int main() {
  size_t cmds_array_len;
  struct sockaddr_in socket_address;
  pthread_t pthread_irc_read_buffer;
  // todo: create another thread to send information to elastic search
  struct irc_lexer *lexer;
  StsHeader *queue;
  struct irc_read_buffer_args thread_args;
  struct irc_message_parser *parser;
  struct message_bus* main_message_bus = allocate_message_bus(5);

  // todo: Let users know in stderr that the bus was not allocated
  if (main_message_bus == NULL) {
    return EXIT_FAILURE;
  }

  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    return EXIT_FAILURE;
  }

  // todo: s/elastic_search/elasticsearch/
  elasticsearch = allocate_elastic_search("elastic_irc_bot", "privmsg");
  if (elasticsearch == NULL) {
    // todo: Let users know in stderr that connetion to elasticsearch didn't work
    return EXIT_FAILURE;
  }

  elastic_connection = elastic_search_connect(elasticsearch, "http://localhost:9200");
  if (elastic_connection == NULL) {
    deallocate_elastic_search(elasticsearch);
    // todo: Let users know in stderr that connetion to elasticsearch didn't work
    return EXIT_FAILURE;
  }

  message_bus_bind_listener(main_message_bus, &write_irc_message_to_elastic_search_listener);
  message_bus_bind_listener(main_message_bus, &irc_ping_pong_listener);

  char *cmds[] = {
      IRC_PASS,
      IRC_NICK,
      IRC_CONNECT,
      IRC_CHANNEL
  };

  signal(SIGKILL, &signal_termination_handler);
  signal(SIGSTOP, &signal_termination_handler);
  signal(SIGTERM, &signal_termination_handler);
  signal(SIGTSTP, &signal_termination_handler);

  queue = StsQueue.create();
  lexer = allocate_irc_lexer(queue);
  if (!lexer) {
    fprintf(stderr, "Unable to create Lexer.\n");
    return EXIT_FAILURE;
  }

  socket_address.sin_family = AF_INET;
  socket_address.sin_addr.s_addr = inet_addr(SERVER_IP);
  socket_address.sin_port = htons(SERVER_PORT);

  socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);

  if (socket_descriptor == -1) {
    fprintf(stderr, "Socket could not be created.\n");
    return EXIT_FAILURE;
  }

  if (inet_pton(AF_INET, SERVER_IP, &socket_address.sin_addr) <= 0) {
    fprintf(stderr, "%s Is an invalid IP address.\n", SERVER_IP);
    return EXIT_FAILURE;
  }

  if (connect(socket_descriptor, (const struct sockaddr *) &socket_address, sizeof(socket_address)) == -1) {
    fprintf(stderr, "Socket could not connect.\n");
    return EXIT_FAILURE;
  }

  thread_args.queue = queue;

  if (pthread_create(&pthread_irc_read_buffer, NULL, &irc_read_buffer_thread, &thread_args)) {
    fprintf(stderr, "Could not create thread.\n");
    return EXIT_FAILURE;
  }

  cmds_array_len = sizeof(cmds) / sizeof(char *);

  for (size_t i = 0; i < cmds_array_len; i++) {
    printf("Sending: %s\n", cmds[i]);
    send(socket_descriptor, cmds[i], strlen(cmds[i]), 0);
  }

  parser = allocate_irc_message_parser(lexer);

  while (true) {
    struct message_envelope* envelope;
    struct irc_message *msg;

    if ((envelope = (struct message_envelope*)malloc(sizeof(struct message_envelope))) == NULL) {
      continue;
    }

    msg = irc_message_parser_parse(parser);
    if (msg != NULL && msg->command != NULL) {
      envelope->message_type = MESSAGE_BUS_TYPE_IRC_MSG;
      envelope->data =  (void *) msg;

      message_bus_send_message(main_message_bus, envelope);
      deallocate_irc_message(msg);
    }

    free(envelope);

    if (irc_read_thread_stopped && !irc_message_parser_can_parse(parser)) {
      break;
    }

    if (!irc_message_parser_can_parse(parser)) {
      usleep(500000);
      continue;
    }
  }

  send(socket_descriptor, "QUIT\r\n", 7, 0);

  if (pthread_join(pthread_irc_read_buffer, NULL)) {
    fprintf(stderr, "Error joining thread\n");
    return EXIT_FAILURE;
  }

  elastic_search_disconnect(elastic_connection);

  curl_global_cleanup();

  deallocate_irc_message_parser(parser);
  shutdown(socket_descriptor, SHUT_RDWR);

  return EXIT_SUCCESS;
}

void *irc_read_buffer_thread(void *thread_args) {
  struct irc_read_buffer_args *args = (struct irc_read_buffer_args *) thread_args;
  ssize_t num_bytes_read;
  int i;
  size_t output_character_position;
  char socket_read_buffer[IRC_BUF_LENGTH << 1];
  char *output_buffer;

  output_character_position = 0;
  output_buffer = (char *) malloc(sizeof(char) * (IRC_BUF_LENGTH - 1));

  while ((num_bytes_read = read(socket_descriptor, socket_read_buffer, (IRC_BUF_LENGTH << 1) - 1)) > 0) {
    for (i = 0; i < num_bytes_read; i++) {

      /**
       * According to the spec, since \r\n will always be a delimiter, then
       * we need to check for the current character in the input buffer against
       * the last character of the output buffer so that when they match
       * exactly, we have a clear line break. This is necessary because of how
       * the internal buffering works for the read() function. It's never
       * guaranteed to nicely buffer things.
       */
      if (socket_read_buffer[i] == '\n' && output_buffer[output_character_position - 1] == '\r') {

        output_buffer[output_character_position - 1] = '\0';

        StsQueue.push(args->queue, output_buffer);

        output_buffer = NULL;
        output_buffer = (char *) malloc(sizeof(char) * (IRC_BUF_LENGTH - 1));

        output_character_position = 0;
        continue;
      }

      output_buffer[output_character_position] = socket_read_buffer[i];
      output_character_position++;
    }
  }

  irc_read_thread_stopped = true;

  return NULL;
}

void signal_termination_handler(int signal_num) {
  send(socket_descriptor, "QUIT\r\n", 7, 0);
  shutdown(socket_descriptor, SHUT_RDWR);
  printf("Sent quit cmd\n");
}

void write_irc_message_to_elastic_search_listener(struct message_envelope *envelope)
{
  struct irc_message* msg;
  char *time_as_string;

  if (envelope->message_type != MESSAGE_BUS_TYPE_IRC_MSG)
    return;

  msg = (struct irc_message*) envelope->data;

  if (!irc_command_is_type(msg->command, IRC_CMD_NAME) || strcasecmp("PRIVMSG", msg->command->command.name.value) != 0) {
    return;
  }

  time_as_string = asctime(gmtime(&msg->command->datetime_created));

  time_as_string[strlen(time_as_string) - 1] = '\0';
  printf("[%s] ", time_as_string);

  if (msg->prefix != NULL) {
    printf("%s -- ", msg->prefix->value);
  }

  if (msg->command->command_type == IRC_CMD_CODE) {
    printf("%i", msg->command->command.code);
  } else {
    printf("%s", msg->command->command.name.value);
  }

  printf(" -- ");

  for (uint8_t i = 0; i < msg->command->parameter_count; i++) {
    printf("%s ", msg->command->parameters[i]->value);
  }

//  elastic_search_insert(elastic_connection, msg);

  printf("\n");
}


void irc_ping_pong_listener(struct message_envelope *envelope)
{
  struct irc_message* msg;

  if (envelope->message_type != MESSAGE_BUS_TYPE_IRC_MSG)
    return;

  msg = (struct irc_message*) envelope->data;

  if (irc_command_is_type(msg->command, IRC_CMD_NAME) && strcasecmp("PING", msg->command->command.name.value) == 0) {
    printf("\nPONG\n");
    send(socket_descriptor, "PONG chat.freenode.net\r\n", 26, 0);
  }
}