#include <sts_queue/sts_queue.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "irc_lexer.h"
#include "irc_token.h"

struct irc_lexer {
  StsHeader *line_buffer;
  char *current_line;
  char current_character;
  size_t current_line_length;
  size_t current_column;
};

static bool __is_space(char character);
static bool __is_colon(char character);
static bool __is_digit(char character);
static bool __is_nospcrlfcl(char character);
static bool __is_letter(char character);
static int __parse_int(struct irc_lexer *lexer);
static char *__parse_word(struct irc_lexer *lexer);
static void __advance(struct irc_lexer *lexer);

struct irc_lexer *allocate_irc_lexer(StsHeader *line_buffer) {
  struct irc_lexer *lexer;

  if ((lexer = (struct irc_lexer *) malloc(sizeof(struct irc_lexer))) == NULL) {
    return NULL;
  }

  lexer->line_buffer = line_buffer;
  lexer->current_line = NULL;
  lexer->current_line_length = 0;
  lexer->current_column = 0;

  return lexer;
}

void deallocate_irc_lexer(struct irc_lexer *lexer) {
  free(lexer->current_line);
  StsQueue.destroy(lexer->line_buffer);
  free(lexer);
}

struct irc_token *irc_lexer_get_next_token(struct irc_lexer *lexer) {
  struct irc_token *tok;
  union irc_token_value tok_value;
  char *current_line;

  if (lexer->current_line != NULL && lexer->current_column > lexer->current_line_length - 1) {
    free(lexer->current_line);
    lexer->current_line = NULL;
    lexer->current_column = 0;
    lexer->current_line_length = 0;

    tok_value.character = '\n';
    return allocate_irc_token(IRC_TOKEN_EOL, tok_value);
  }

  if (lexer->current_line == NULL) {
    current_line = StsQueue.pop(lexer->line_buffer);

    if (current_line == NULL) {
      tok_value.character = '\0';
      tok = allocate_irc_token(IRC_TOKEN_EOF, tok_value);
      return tok;
    }

    lexer->current_line = current_line;
    lexer->current_line_length = strlen(current_line);
    lexer->current_character = lexer->current_line[0];
  }

  if (__is_colon(lexer->current_character)) {
    tok_value.character = lexer->current_character;
    __advance(lexer);
    return allocate_irc_token(IRC_TOKEN_COLON, tok_value);
  }

  if (__is_nospcrlfcl(lexer->current_character)) {
    tok_value.character = lexer->current_character;
    __advance(lexer);
    return allocate_irc_token(IRC_TOKEN_NOSPCRLFCL, tok_value);
  }

  if (__is_letter(lexer->current_character)) {
    tok_value.character = lexer->current_character;
    __advance(lexer);
    return allocate_irc_token(IRC_TOKEN_LETTER, tok_value);
  }

  if (__is_space(lexer->current_character)) {
    tok_value.character = '\x20';
    __advance(lexer);
    return allocate_irc_token(IRC_TOKEN_SPACE, tok_value);
  }

//  if (__is_digit(lexer->current_character)) {
//    tok_value.integer = __parse_int(lexer);
//    return allocate_irc_token(IRC_TOKEN_INTEGER_LITERAL, tok_value);
//  }

  tok_value.character = '\0';
  __advance(lexer);
  return allocate_irc_token(IRC_TOKEN_NONE, tok_value);
}

char *irc_lexer_get_current_line(struct irc_lexer *lexer) {
  char *dest = (char *) malloc(sizeof(char) * lexer->current_line_length);
  strncpy(dest, lexer->current_line, lexer->current_line_length);
  return dest;
}

struct irc_token *irc_lexer_peek_next_token(struct irc_lexer *lexer) {
  struct irc_token *next_token;
  char *current_line;
  size_t current_line_length;
  size_t current_column;
  char current_character;

  // copy current state
  current_line_length = lexer->current_line_length;
  current_column = lexer->current_column;
  current_character = lexer->current_character;

  current_line = (char *) malloc(sizeof(char) * lexer->current_line_length);
  strncpy(current_line, lexer->current_line, lexer->current_line_length);

  // get next token
  next_token = irc_lexer_get_next_token(lexer);

  // free current line to reset state
  free(lexer->current_line);

  // Reset state
  // There's a problem. We hanlded the case where the next token is on the next
  // line of the buffer, and we successfully revert back. The problem is, we
  // need a way to put the next line back into the first queue if it switched.
  lexer->current_line = current_line;
  lexer->current_line_length = current_line_length;
  lexer->current_column = current_column;
  lexer->current_character = current_character;

  return next_token;
}

static bool __is_space(char character) {
  return character == '\x20';
}

static bool __is_colon(char character) {
  return character == ':';
}

static bool __is_digit(char character) {
  return character >= '\x30' && character <= '\x39';
}

static bool __is_nospcrlfcl(char character) {
  return
      (character >= '\x01' && character <= '\x09') ||
      (character >= '\x0B' && character <= '\x0C') ||
      (character >= '\x0E' && character <= '\x1F') ||
      (character >= '\x21' && character <= '\x39') ||
      (character >= '\x3B' && character <= '\x7F');
}

static bool __is_letter(char character) {
  return (character >= '\x41' && character <= '\x5A') ||
      (character >= '\x61' && character <= '\x7A');
}

static int __parse_int(struct irc_lexer *lexer) {
  int result = 0;

  while (__is_digit(lexer->current_character)) {
    result = result * 10 + (int) lexer->current_character - (int) '0';
    __advance(lexer);
  }

  return result;
}

void __advance(struct irc_lexer *lexer) {
  lexer->current_column++;

  if (lexer->current_column > lexer->current_line_length) {
    lexer->current_character = '\0';
    return;
  }

  lexer->current_character = lexer->current_line[lexer->current_column];
}
