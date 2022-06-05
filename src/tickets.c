#include <stdio.h>
#include "strings.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    unsigned char hash[32];
    unsigned char used;
} ticket;

#define TICKET_SIZE 33
#define MAX_TICKET 100
static ticket tickets[TICKET_SIZE * MAX_TICKET];
static size_t tickets_len = 0;

// Initialize Vote
// At the start, no votes

// 
void vote_init() { }

// 
void add_voting_ticket(unsigned char* hash) {
    if (tickets_len >= MAX_TICKET) return;
    tickets[tickets_len].used = 0;
    tickets[tickets_len].hash =
}