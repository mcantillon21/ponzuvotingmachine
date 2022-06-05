#include "vote.h"

// External Libraries
#include "gl.h"
#include "gpio.h"
#include "keyboard.h"
#include "printf.h"
#include "strings.h"
#include "malloc.h"
#include <stdbool.h>
#include <stdarg.h>
#include "sha256.h"
#include "interrupts.h"

// Project Imports
#include "screen.h"

typedef struct {
    unsigned char hash[32];
    unsigned char used;
} ticket;

#define MAX_PASS 30
#define TICKET_SIZE 33
#define MAX_TICKET 100
#define CERT_SIZE 6

// Tickets
static ticket tickets[TICKET_SIZE * MAX_TICKET];
static size_t ticket_iter = 0;
static int selected_ticket = -1;
static int selected_cert = 2;

// Votes
static leaf vote_leafs[30];
static vote_merkle *vote_merkle_tree;
static int nonce;
static size_t vote_iter = 0;
static node *curr_merkle_proof;


// Current Selected Password
static char curr_pass[MAX_PASS] = "";
static char current_cert[CERT_SIZE] = "";
static char cert_input[CERT_SIZE] = "";

void add_ticket(const char *);
void init_voting(void);
void handle_event(void);
void move(unsigned int direction);
bool vote(ticket *vote_ticket, int candidate);

/*
 * Main
 */

void main(void)
{
    gpio_init();
    add_ticket("v");
    add_ticket("p");
    add_ticket("a");
    add_ticket("b");
    add_ticket("c");

    init_voting();
}

/*
 * TICKET FUNCTIONS
 */

int check_cert(char * cert) {
    char cert_bytes[(strlen(cert) + 1) / 2];
    hex_to_bytes(cert, cert_bytes, CERT_SIZE);

    unsigned int merkle_height = vote_merkle_tree->height;
    unsigned int num_leaf = 1 << merkle_height;

    for (int leaf_index = 0; leaf_index < num_leaf; leaf_index++) {
        node* curr = &vote_merkle_tree->nodes[num_leaf - 1 + leaf_index];
        if (cmp((char *) curr, cert_bytes, CERT_SIZE / 2)) {
            return leaf_index;
        }
    }
    return -1;
}

// Checks for valid vote ticket
int check_ticket(const char * pass) {
    char hash_pass[32];
    SHA256(pass, strlen(pass), hash_pass);
    // SHA256((const unsigned char*) pass, strlen(hash_pass), hash_pass);
    for (int i = 0; i < ticket_iter; i++) {
        if (tickets[i].used == 0 && cmp((char *) tickets[i].hash, hash_pass, 32)) {
            return i;
        }
    }
    return -1;
}

void create_ticket(const char * pass, ticket * new_ticket) {
    char hash[32];
    SHA256(pass, strlen(pass), hash);
    new_ticket->used = 0; 
    memcpy(new_ticket->hash, hash, 32);
}

void add_ticket(const char * pass) {
    create_ticket(pass, &tickets[ticket_iter++]);
}

/*
 * Main Handlers
 */
void draw_screen(void) {
    switch (get_selected_screen()) {
        case Home: 
            draw_home_screen();
            break;
        case Auth:
            draw_auth_screen(curr_pass);
            break;
        case Vote:
            draw_vote_screen();
            break;
        case Admin:
            // draw_admin_screen();
            break;
        case Certificate:
            draw_cert_screen(current_cert);
            break;
        case FraudProof:
            draw_fraud_proof_screen(cert_input);
            break;
        case Merkle:
            draw_fraud_visual_screen(curr_merkle_proof, vote_merkle_tree, selected_cert);
            break;
        case Results:
            printf("new vote: %d", vote_iter);
            draw_results_screen(vote_iter, vote_merkle_tree);
            break;
    }
    gl_swap_buffer();
}

#define num_leaves(tree) (1 << tree->height )

void handle_event() {
    switch (get_selected()) {
        case Back:
            switch_screen(Home, AdminBox);
            break;
        case VoteBox:
            switch_screen(Auth, None);
            break;
        case Candidate1:
            set_selected_candidate(Candidate1);
            break;
        case Candidate2:
            set_selected_candidate(Candidate2);
            break;
        case SubmitBox:
            if (get_selected_candidate() == -1) break;
            vote(&tickets[selected_ticket], (get_selected_candidate() == Candidate1 ? 0 : 1));

            free(vote_merkle_tree);
            vote_merkle_tree = create_merkle_tree(vote_leafs, vote_iter);

            switch_screen(Certificate, CertificateBox);
            bytes_to_hex((char *) &vote_merkle_tree->nodes[num_leaves(vote_merkle_tree) + vote_iter - 2], current_cert, CERT_SIZE / 2);
            break;
        case CertificateBox:
            switch_screen(Home, AdminBox);
            break;
        case FraudProofBox:
            switch_screen(FraudProof, FraudProofBox);
            break;
        case MerkleBox:
            switch_screen(Home, AdminBox);
            break;
        case ResultsBox:
            switch_screen(Home, AdminBox);
            break;
        case SelectResultsBox:
            switch_screen(Results, ResultsBox);
            break;
    }
}

/*
 * Screen Handlers 
 */
void handle_move_screen(unsigned int key) {
    switch (key) {
        case PS2_KEY_ARROW_LEFT:
            move(0);
            break;     
        case PS2_KEY_ARROW_RIGHT:
            move(1);
            break;
        case PS2_KEY_ARROW_UP:
            move(2);
            break;
        case PS2_KEY_ARROW_DOWN:
            move(3);
            break;
        case '\n':
            handle_event();
            break;
    }
}

void handle_auth_screen() {
    // Clear previous passcode
    memset(curr_pass, '\0', MAX_PASS);
    draw_auth_screen(curr_pass);
    gl_swap_buffer();

    unsigned int i = 0;
    unsigned char key = keyboard_read_next();
    while (key != '\n') {
        if (key == '\b') {
            if (i != 0) curr_pass[--i] = '\0';
        } else {
            curr_pass[i++] = key;
        }
        draw_auth_screen(curr_pass);
        gl_swap_buffer();
        key = keyboard_read_next();
    }
    curr_pass[i] = '\0';
    selected_ticket = check_ticket(curr_pass);
    if (selected_ticket != -1) {
        switch_screen(Vote, Back);
        set_selected_candidate(None);
        memset(curr_pass, '\0', MAX_PASS);
    } else {
        double_clear();
        handle_auth_screen(curr_pass);
    }
}

void handle_fraud_screen() {
    // Clear previous passcode
    memset(cert_input, '\0', CERT_SIZE);
    draw_fraud_proof_screen(cert_input);
    gl_swap_buffer();

    unsigned int i = 0;
    unsigned char key = keyboard_read_next();
    while (key != '\n') {
        if (key == '\b') {
            if (i != 0) cert_input[--i] = '\0';
        } else {
            if (i < CERT_SIZE) cert_input[i++] = key;
        }
        draw_fraud_proof_screen(cert_input);
        gl_swap_buffer();
        key = keyboard_read_next();
    }
    selected_cert = check_cert(cert_input);
    if (selected_cert != -1) {
        curr_merkle_proof = create_merkle_proof(vote_merkle_tree, selected_cert);
    }

    switch_screen(Merkle, MerkleBox);
    memset(cert_input, '\0', CERT_SIZE);
}

bool vote(ticket* vote_ticket, int candidate) {
    if (candidate != 1 && candidate != 0) return false;
    if (vote_ticket->used == 1) return false;

    // Add vote leaf
    leaf * vote_leaf = &vote_leafs[vote_iter++];
    printf("new vote: %d", vote_iter);
    SHA256((const char *) &nonce, 4, vote_leaf->randomness); nonce++;
    memset(vote_leaf->sig, 0, 32);
    memcpy(vote_leaf->hash, vote_ticket->hash, 32);
    vote_leaf->vote = (char) candidate;

    // Use ticket
    vote_ticket->used = 1;

    printf("new vote: %d", vote_iter);

    return true;
}

/*
 * Init and store control flow
 */
void init_voting(void) {
    vote_merkle_tree = malloc(8);

    interrupts_init();
    screen_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
    interrupts_global_enable(); // everything fully initialized, now turn on interrupts
    double_clear();
    draw_screen();

    while (1)
    {
        printf("new vote: %d", vote_iter);
        unsigned char key = keyboard_read_next();

        switch (get_selected_screen()) {
            case Home: 
                handle_move_screen(key);
                break;
            case Auth:
                handle_auth_screen(key);
                break;
            case Vote:
                handle_move_screen(key);
                break;
            case Certificate:
                handle_move_screen(key);
                break;
            case FraudProof:
                handle_fraud_screen(key);
                break;
            case Admin:
                // handle_admin_screen();
                break;
            case Merkle:
                handle_move_screen(key);
                break;
            case Results:
                handle_move_screen(key);
                break;
        }
        draw_screen();
    }
}
