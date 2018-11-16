#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <sts_queue/sts_queue.h>
#include <curl/curl.h>

#include "irc_lexer.h"
#include "irc_token.h"
#include "irc_message_parser.h"
#include "irc_message.h"
#include "message_bus.h"
#include "elasticsearch.h"

#include "settings.h"

#define MESSAGE_BUS_TYPE_IRC_MSG 1

static int socket_descriptor;
static volatile bool irc_read_thread_stopped = false;

static struct elasticsearch* elasticsearch;
static struct elasticsearch_connection* elastic_connection;

void log_irc_server_privmsg(struct message_envelope *envelope);
void pong_irc_server(struct message_envelope *envelope);
void log_irc_server_errors(struct message_envelope *envelope);
void log_irc_server_info(struct message_envelope *envelope);

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

  elasticsearch = allocate_elasticsearch("elastic_irc_bot", "privmsg");
  if (elasticsearch == NULL) {
    // todo: Let users know in stderr that connetion to elasticsearch didn't work
    return EXIT_FAILURE;
  }

  elastic_connection = elasticsearch_connect(elasticsearch, ELASTICSEARCH_BASE_URL);
  if (elastic_connection == NULL) {
    deallocate_elasticsearch(elasticsearch);
    // todo: Let users know in stderr that connetion to elasticsearch didn't work
    return EXIT_FAILURE;
  }

  message_bus_bind_listener(main_message_bus, &log_irc_server_privmsg);
  message_bus_bind_listener(main_message_bus, &pong_irc_server);
  message_bus_bind_listener(main_message_bus, &log_irc_server_errors);
  message_bus_bind_listener(main_message_bus, &log_irc_server_info);

  char *cmds[] = {
      "PASS " IRC_USER_PASS "\r\n",
      "NICK " IRC_USER_NICK "\r\n",
      "USER " IRC_USER_NICK " " IRC_USER_SERVER_IP " " IRC_SERVER_IP " :" IRC_USER_REALNAME  "\r\n",
      "JOIN " IRC_CHANNEL "\r\n"
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
  socket_address.sin_addr.s_addr = inet_addr(IRC_SERVER_IP);
  socket_address.sin_port = htons(IRC_SERVER_PORT);

  socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);

  if (socket_descriptor == -1) {
    fprintf(stderr, "Socket could not be created.\n");
    return EXIT_FAILURE;
  }

  if (inet_pton(AF_INET, IRC_SERVER_IP, &socket_address.sin_addr) <= 0) {
    fprintf(stderr, "%s Is an invalid IP address.\n", IRC_SERVER_IP);
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
    printf("Sending: %s", cmds[i]);
    send(socket_descriptor, cmds[i], strlen(cmds[i]) * sizeof(char), 0);
  }

  parser = allocate_irc_message_parser(lexer);

  while (true) {
    struct message_envelope* envelope;
    struct irc_message *msg;

    if ((envelope = (struct message_envelope*) malloc(sizeof(struct message_envelope))) == NULL) {
      continue;
    }

    msg = irc_message_parser_parse(parser);
    if (msg != NULL) {
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

  send(socket_descriptor, "QUIT\r\n", 6 * sizeof(char), 0);

  if (pthread_join(pthread_irc_read_buffer, NULL)) {
    fprintf(stderr, "Error joining thread\n");
    return EXIT_FAILURE;
  }

  deallocate_message_bus(main_message_bus);
  elasticsearch_disconnect(elastic_connection);

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
  output_buffer = (char *) calloc(IRC_BUF_LENGTH, sizeof(char));

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
        output_buffer = (char *) calloc(IRC_BUF_LENGTH, sizeof(char));

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
  printf("Sending: QUIT\n");
  send(socket_descriptor, "QUIT\r\n", 6 * sizeof(char), 0);
  shutdown(socket_descriptor, SHUT_RDWR);
}

void log_irc_server_privmsg(struct message_envelope *envelope)
{
  struct irc_message* msg;

  if (envelope->message_type != MESSAGE_BUS_TYPE_IRC_MSG)
    return;

  msg = (struct irc_message*) envelope->data;

  if (!irc_command_name_is(msg->command, "PRIVMSG"))
    return;

  elasticsearch_insert(elastic_connection, msg);
}

void pong_irc_server(struct message_envelope *envelope)
{
  struct irc_message* msg;
  char *cmd = "PONG " IRC_SERVER_IP "\r\n";

  if (envelope->message_type != MESSAGE_BUS_TYPE_IRC_MSG)
    return;

  msg = (struct irc_message*) envelope->data;

  if (!irc_command_name_is(msg->command, "PING"))
    return;

  printf("Sending: PONG\n");
  send(socket_descriptor, cmd, strlen(cmd) * sizeof(char), 0);
}

void log_irc_server_errors(struct message_envelope *envelope)
{
  struct irc_message* msg;

  if (envelope->message_type != MESSAGE_BUS_TYPE_IRC_MSG)
    return;

  msg = (struct irc_message*) envelope->data;

  if (!irc_command_name_is(msg->command, "ERROR")) {
    return;
  }

  irc_message_pretty_print(msg, stderr);
}

void log_irc_server_info(struct message_envelope *envelope)
{
  struct irc_message* msg;

  if (envelope->message_type != MESSAGE_BUS_TYPE_IRC_MSG)
    return;

  msg = (struct irc_message*) envelope->data;

  if (irc_command_name_is(msg->command, "ERROR") ||
      irc_command_name_is(msg->command, "PRIVMSG")) {
    return;
  }

  irc_message_pretty_print(msg, stdout);
}
