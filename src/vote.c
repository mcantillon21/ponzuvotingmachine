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
    unsigned int* intervals;
    char name[20];
} ticket;

#define MAX_PASS 30
#define TICKET_SIZE 33
#define MAX_TICKET 100
#define CERT_SIZE 6
#define BUFFER_SIZE 40
#define ERROR_SIZE 40

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
static char voter_name[MAX_PASS] = "";
static char admin_input[MAX_PASS] = "";
static char admin_pass[MAX_PASS] = "";
static char pass_error[ERROR_SIZE] = "";
static char success_phrase[ERROR_SIZE] = "";

static unsigned int last_time = 0;

void add_ticket(const char * pass, unsigned int * intervals);
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
int check_ticket(const char * pass, unsigned int * intervals) {
    char hash_pass[32];
    SHA256(pass, strlen(pass), hash_pass);
    // SHA256((const unsigned char*) pass, strlen(hash_pass), hash_pass);
    for (int i = 0; i < ticket_iter; i++) {
        // Checks if the hashes (sentences) match
        if (!(tickets[i].used == 0 && cmp((char *) tickets[i].hash, hash_pass, 32))) {
            continue;
        }

        // Check whether RMS is within acceptable threshold
        int iter_max = (strlen(pass) - 1 > BUFFER_SIZE) ? BUFFER_SIZE : strlen(pass) - 1;
        unsigned int squared_error = 0;
        unsigned int THRESHOLD = 10000;
        for (int j = 0; j < iter_max; j++) {
            squared_error += intervals[j] * tickets[i].intervals[j];
        }
        if (squared_error < THRESHOLD * iter_max) {
            return i;
        }
    }

    return -1;
}

void create_ticket(const char * pass, ticket * new_ticket, unsigned int * intervals) {
    char hash[32];
    SHA256(pass, strlen(pass), hash);
    new_ticket->used = 0; 
    memcpy(new_ticket->hash, hash, 32);
    new_ticket->intervals = intervals;
    memcpy(new_ticket->name, voter_name, strlen(voter_name));
    new_ticket->name[strlen(voter_name)] = '\0';
}

void add_ticket(const char * pass, unsigned int * intervals) {
    create_ticket(pass, &tickets[ticket_iter++], intervals);
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
            draw_auth_screen(curr_pass, pass_error, tickets[selected_ticket].name);
            break;
        case Vote:
            draw_vote_screen(tickets[selected_ticket].name);
            break;
        case Admin:
            draw_admin_screen(admin_input, success_phrase);
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
        case AdminLogin:
            draw_admin_auth_screen(admin_pass);
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
        case AdminBox:
            switch_screen(AdminLogin, AdminLoginScreenBox);
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

unsigned char keyboard_read_next_char() {
    key_out_t key = keyboard_read_next();
    return key.elem;
}

void handle_auth_screen() {
    // Clear previous passcode
    memset(curr_pass, '\0', MAX_PASS);
    draw_auth_screen(curr_pass, pass_error, tickets[selected_ticket].name);
    gl_swap_buffer();
    draw_auth_screen(curr_pass, pass_error, tickets[selected_ticket].name);

    unsigned int i = 0;
    key_out_t key_out = keyboard_read_next();
    char key = key_out.elem;
    last_time = key_out.time;

    unsigned int * intervals = malloc(BUFFER_SIZE * 4);
    size_t int_iter = 0;

    unsigned int x = (unsigned int) em(20);
    unsigned int y = (unsigned int) em(30);
    unsigned int char_width = gl_get_char_width();
    unsigned int char_height = gl_get_char_height();

    while (key != '\n') {
        if (key == PS2_KEY_ESC) {
            switch_screen(Home, AdminBox);
            pass_error[0] = '\0';
            return;
        }

        if (key == '\b') {} 
        else {
            if (i < MAX_PASS) {
                curr_pass[i++] = key;
                gl_draw_char(x, y, key, GL_BLACK);
                gl_swap_buffer();
                gl_draw_char(x, y, key, GL_BLACK);
                x += char_width;
            }
        }
        key_out = keyboard_read_next();
        key = key_out.elem;
        if (key != '\b' && key != '\n') {
            intervals[int_iter++] = (key_out.time - last_time) / 1000;
         }
        last_time = key_out.time;
    }
    curr_pass[i] = '\0';
    selected_ticket = check_ticket(curr_pass, intervals);
    if (selected_ticket != -1) {
        switch_screen(Vote, Back);
        set_selected_candidate(None);
        memset(curr_pass, '\0', MAX_PASS);
        pass_error[0] = '\0';
    } else {
        memcpy(pass_error, "Authentication Failed", strlen("Authentication Failed"));
        pass_error[strlen("Authentication Failed")] = '\0';
    }
}

void handle_fraud_screen() {
    // Clear previous passcode
    memset(cert_input, '\0', CERT_SIZE);
    draw_fraud_proof_screen(cert_input);
    gl_swap_buffer();
    

    unsigned int i = 0;
    unsigned char key = keyboard_read_next_char();
    while (key != '\n') {
        if (key == '\b') {
            if (i != 0) cert_input[--i] = '\0';
        } else {
            if (i < CERT_SIZE) cert_input[i++] = key;
        }
        draw_fraud_proof_screen(cert_input);
        gl_swap_buffer();
        key = keyboard_read_next_char();
    }
    selected_cert = check_cert(cert_input);
    if (selected_cert != -1) {
        curr_merkle_proof = create_merkle_proof(vote_merkle_tree, selected_cert);
    }

    switch_screen(Merkle, MerkleBox);
    memset(cert_input, '\0', CERT_SIZE);
}

void handle_admin_auth_screen() {
    // Clear previous passcode
    memset(admin_pass, '\0', MAX_PASS);
    draw_admin_auth_screen(admin_pass);
    gl_swap_buffer();
    draw_admin_auth_screen(admin_pass);

    unsigned int i = 0;
    unsigned char key = keyboard_read_next_char();
    while (key != '\n') {
        if (key == PS2_KEY_ESC) {
            switch_screen(Home, AdminBox);
            return;
        }
        if (key == '\b') {
            if (i != 0) admin_pass[--i] = '\0';
        } else {
            if (i < CERT_SIZE) admin_pass[i++] = key;
        }
        draw_admin_auth_screen(admin_pass);
        gl_swap_buffer();
        key = keyboard_read_next_char();
    }
    admin_input[i] = '\0';

    if (strcmp(admin_pass, "pass") == 0) {
        printf("authentication pass");
        switch_screen(Admin, AdminBox);
    } else {
        printf("authentication failed");
    }
}

void handle_admin_screen() {
    // Clear previous passcode
    memset(admin_input, '\0', MAX_PASS);
    memset(voter_name, '\0', MAX_PASS);
    draw_admin_screen(admin_input, success_phrase);
    gl_swap_buffer();
    draw_admin_screen(admin_input, success_phrase);

    unsigned int i = 0;

    unsigned int x = (unsigned int) em(20);
    unsigned int y = (unsigned int) em(30);
    unsigned int char_width = gl_get_char_width();
    unsigned int char_height = gl_get_char_height();

    key_out_t key_out = keyboard_read_next();
    char key = key_out.elem;

    while (key != '\n') {
        if (key == PS2_KEY_ESC) {
            success_phrase[0] = '\0';
            switch_screen(Home, AdminBox);
            return;
        }
        if (key == '\b') {
            i--;
            x -= char_width;
            gl_draw_rect(x, y, char_width, char_height, GL_WHITE);
            gl_swap_buffer();
            gl_draw_rect(x, y, char_width, char_height, GL_WHITE);
        } 
        else {
            if (i < MAX_PASS) {
                voter_name[i++] = key;
                gl_draw_char(x, y, key, GL_BLACK);
                gl_swap_buffer();
                gl_draw_char(x, y, key, GL_BLACK);
                x += char_width;
            }
        }

        key_out = keyboard_read_next();
        key = key_out.elem;
    }
    voter_name[i] = '\0';

    i = 0;
    key_out = keyboard_read_next();
    key = key_out.elem;
    last_time = key_out.time;

    unsigned int * intervals = malloc(BUFFER_SIZE * 4);
    size_t int_iter = 0;

    x = (unsigned int) em(20);
    y = (unsigned int) em(50);

    while (key != '\n') {
        if (key == PS2_KEY_ESC) {
            success_phrase[0] = '\0';
            switch_screen(Home, AdminBox);
            return;
        }
        if (key == '\b') {} 
        else {
            if (i < MAX_PASS) {
                admin_input[i++] = key;
                gl_draw_char(x, y, key, GL_BLACK);
                gl_swap_buffer();
                gl_draw_char(x, y, key, GL_BLACK);
                x += char_width;
            }
        }

        key_out = keyboard_read_next();
        key = key_out.elem;
        if (key != '\b' && key != '\n') {
            intervals[int_iter++] = (key_out.time - last_time) / 1000;
         }
        last_time = key_out.time;
    }
    admin_input[i] = '\0';

    printf("%s\n", admin_input);
    printf("%d %d\n", strlen(admin_input), int_iter);
    printf("%d", intervals[0]);
    
    draw_admin_screen(admin_input, success_phrase);
    gl_swap_buffer();

    memcpy(success_phrase, "Registration success", strlen("Registration success"));
    success_phrase[strlen("Registration success")] = '\0';
    add_ticket((const char *) admin_input, intervals);
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
        switch (get_selected_screen()) {
            case AdminLogin:
                handle_admin_auth_screen();
                continue;
            case Admin:
                handle_admin_screen();
                continue;
            case Auth:
                handle_auth_screen();
                continue;
            case FraudProof:
                handle_fraud_screen();
                continue;
        }
 
        draw_screen();
        key_out_t key_out = keyboard_read_next();
        char key = key_out.elem;

        switch (get_selected_screen()) {
            case Home:
                handle_move_screen(key);
                break;
            case Vote:
                handle_move_screen(key);
                break;
            case Certificate:
                handle_move_screen(key);
                break;
            case Merkle:
                handle_move_screen(key);
                break;
            case Results:
                handle_move_screen(key);
                break;
        }
    }
}
