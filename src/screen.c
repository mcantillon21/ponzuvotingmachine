#include "gl.h"
#include "screen.h"
#include "strings.h"
#include "printf.h"

static unsigned int selected = AdminScreenBox;
static unsigned int selected_screen = Admin;
static unsigned int selected_candidate = None;

static color_t c_foreground = 0xFFFFFFFF;
static color_t c_background = 0xFF0e380e;
static unsigned int width = 480;
static unsigned int height = 400;

int unsigned_to_base(char *buf, 
                     size_t bufsize, 
                     unsigned int val, 
                     int base, size_t 
                     min_width);

/*
 * Boxes Map
 */

static key_mapping key_map[] = {
    {None, None, None, None}, // None
    {None, None, None, Candidate1}, // Back
    {None, Candidate2, Back, SubmitBox}, // Candidate1
    {Candidate1, None, Back, SubmitBox}, // Candidate2
    {AdminBox, None, None, SelectResultsBox}, // VoteBox
    {None, VoteBox, None, FraudProofBox}, // AdminBox
    {None, SelectResultsBox, AdminBox, None}, // FraudProofBox
    {FraudProofBox, None, VoteBox, None}, // SelectResultsBox
    {None, None, Candidate1, None}, // SubmitBox
    {None, None, None, None}, // CertificateBox
    {None, None, None, None}, // MerkleBox
    {None, None, None, None}, // ResultsBox
    {None, None, None, None}, // AdminScreenBox
    {None, None, None, None}, // AdminLoginScreenBox
};

void screen_init(void) {
    gl_init(width, height, GL_DOUBLEBUFFER); 
}

/*
 * Getters Setters
 */

unsigned int get_selected(void) {
    return selected;
}

void set_selected(unsigned int new_selected) {
    selected = new_selected;
}

unsigned int get_selected_candidate(void) {
    return selected_candidate;
}

void set_selected_candidate(unsigned int new_candidate) {
    selected_candidate = new_candidate;
}

unsigned int get_selected_screen(void) {
    return selected_screen;
}

void set_selected_screen(unsigned int new_screen) {
    selected_screen = new_screen;
}

/*
 * Screen Draw Functions
 */

void draw_vote_screen(char* name) {
    draw_title_block(name);
    draw_back_block(selected);
    draw_matt_block(selected, selected_candidate);
    draw_christos_block(selected, selected_candidate);
    draw_submit_vote_block(selected, selected_candidate);
}

void draw_auth_screen(char * curr_pass, char* pass_error, char* voter_name) {
    gl_draw_rect(em(15), em(10), em(90), em(50), GL_WHITE);
    gl_draw_string(em(20), em(20), "Enter Vote Phrase:", GL_BLACK);

    gl_draw_string(em(20), em(45), pass_error, GL_RED);

    gl_draw_rect(em(15), em(70), em(90), em(25), GL_WHITE);
    gl_draw_string(em(35), em(80), "Press Enter", GL_BLACK);
}

void draw_home_screen(void) {
    draw_voting_block(selected);
    draw_admin_block(selected);
    draw_fraud_proof_block(selected);
    draw_results_block();
}

void draw_cert_screen(char * cert) {
    draw_certificate_block(cert);
}

void draw_admin_auth_screen(char * curr_admin_pass) {
    unsigned int len = strlen(curr_admin_pass);
    char display[len + 1];
    memset(display, '*', len);
    display[len] = '\0';

    gl_draw_rect(em(15), em(10), em(90), em(40), GL_WHITE);
    gl_draw_string(em(20), em(20), "Enter Password:", GL_BLACK);
    gl_draw_string(em(35), em(40), display, GL_BLACK);

    gl_draw_rect(em(15), em(60), em(90), em(25), GL_WHITE);
    gl_draw_string(em(35), em(70), "Press Enter", GL_BLACK);
}

void draw_admin_screen(char * curr_pass_input, char * success) {
    gl_draw_rect(em(15), em(10), em(90), em(70), GL_WHITE);
    gl_draw_string(em(20), em(20), "Enter Name:", GL_BLACK);
    gl_draw_string(em(20), em(40), "Enter New Vote Phrase", GL_BLACK);

    gl_draw_string(em(20), em(60), success, 0x4BB543);
}

void draw_fraud_proof_screen(char * cert) {
    gl_draw_rect(em(5), em(5), em(110), em(40), GL_WHITE);
    gl_draw_string(em(10), em(10), "Enter Certificate:", GL_BLACK);
    gl_draw_string(em(10), em(20), cert, GL_BLACK);

    gl_draw_rect(em(5), em(60), em(110), em(25), GL_WHITE);
    gl_draw_string(em(35), em(70), "Press Enter", GL_BLACK);
}

void merkle_rect(int x, int y, color_t color) {
    gl_draw_rect(x - em(2), y - em(2), em(4), em(4), color);
}

#define left_child(i) 2 * i + 1
#define right_child(i) 2 * i + 2
#define parent(i) (i - 1) / 2
#define is_right(i) i % 2 == 0

void draw_fraud_visual_screen(node* merkle_proof, vote_merkle* merkle, int node_index) {
    if (node_index == -1) {
        gl_draw_rect(em(5), em(5), em(110), em(90), GL_WHITE);
        gl_draw_string(em(10), em(10), "Invalid Certificate", GL_BLACK);

        gl_draw_string(em(10), em(20), "Press Enter to Return", GL_BLACK);
        gl_draw_string(em(10), em(30), "Home", GL_BLACK);

     } else {
        gl_draw_rect(em(5), em(5), em(110), em(90), GL_WHITE);
        gl_draw_string(em(10), em(10), "Valid Certificate!:", GL_BLACK);

        merkle_rect(em(20), em(76), GL_RED);
        merkle_rect(em(20), em(83), 0xFF4281f5);
        gl_draw_string(em(30), em(74), "Merkle Proof Nodes", GL_BLACK);
        gl_draw_string(em(30), em(81), "Your Path", GL_BLACK);


        int starting = em(55);
        int curr_y = em(20);
        int box_sep = em(10);
        int starting_sep = box_sep * 2;
        int row_sep = box_sep * 8;

        unsigned int tree_height = merkle->height;
        unsigned int num_leafs = 1 << tree_height;

        printf("Merkle Leaf:\n");
        print_bytes(&merkle->nodes[num_leafs - 1 + node_index], 32);
        printf("Merkle Proof:\n");
        for (int i = 0; i < merkle->height; i++) {
            print_bytes(&merkle_proof[i], 32);
        }
        printf("Merkle Root:\n");
        print_bytes(&merkle->nodes[0], 32);
 

        // Colour Nodes 
        size_t cols[num_leafs * 2 - 1];
        unsigned int curr_node = node_index + num_leafs - 1;
        memset(cols, 0, (num_leafs * 2 - 1) * 4);

        for (int i = 0; i < tree_height; i++) {
            unsigned int parent = parent(curr_node);
            unsigned int neighbour = is_right(curr_node) ? left_child(parent) : right_child(parent);
            cols[parent] = 1;
            cols[neighbour] = 2;
            curr_node = parent;
        }
        cols[num_leafs - 1 + node_index] = 1;

        color_t color_map[] = {GL_BLACK, 0xFF4281f5, GL_RED};

        unsigned int true_index = 0;
        unsigned leaf_index = 0;
 
        for (int i = 0; i < tree_height + 1; i++) {
            unsigned int num_nodes = 1 << i;
            int currx = starting; 
            for (int j = 0; j < num_nodes; j++) {
                color_t color = color_map[cols[true_index]];
                merkle_rect(currx, curr_y, color);
                currx += row_sep;
                true_index++;
            }

            starting -= starting_sep;
            starting_sep /= 2;
            row_sep /= 2;
            curr_y += em(10);
        }        
    }
}

void draw_results_screen(unsigned int num_votes, vote_merkle* merkle_tree) {
    printf("%d\n", num_votes);
    unsigned int vote_count = merkle_tree->nodes[0].vote_count;
    unsigned int christos_vote = vote_count;
    unsigned int matt_vote = num_votes - vote_count;

    gl_draw_rect(em(5), em(5), em(110), em(90), GL_WHITE);

    const int buf_size = 10;
    char christos_vote_str[buf_size];
    char matt_vote_str[buf_size];

    unsigned_to_base(christos_vote_str, buf_size, christos_vote, 10, 1);
    unsigned_to_base(matt_vote_str, buf_size, matt_vote, 10, 1);

    gl_draw_string(em(10), em(10), "Matt Votes", GL_BLACK);
    gl_draw_string(em(10), em(20), matt_vote_str, GL_BLACK);

    gl_draw_string(em(10), em(35), "Christos Votes:", GL_BLACK);
    gl_draw_string(em(10), em(45), christos_vote_str, GL_BLACK);

    char root_hash[21];
    bytes_to_hex(merkle_tree->nodes[0].hash, root_hash, 10);
    root_hash[20] = '\0';

    gl_draw_string(em(10), em(60), "Merkle Root", GL_BLACK);
    gl_draw_string(em(10), em(70), "(First 20 Hex):", GL_BLACK);
    gl_draw_string(em(10), em(80), root_hash, GL_BLACK);
}

void double_clear(void) {
    gl_clear(c_background);
    gl_swap_buffer();
    gl_clear(c_background);
}

void switch_screen(unsigned int next_screen, unsigned int next_box) {
    selected_screen = next_screen;
    selected = next_box;
    double_clear();
}

/*
 * Block Draw Functions
 */


// --------------------- VOTE PAGE ----------------------

void draw_title_block(char* name)
{
    color_t COLOR = GL_WHITE;
    gl_draw_rect(em(30), em(5), em(80), em(11), COLOR);
    char buf[100];
    snprintf(buf, 100,  "Welcome, %s", name);
    gl_draw_string(em(35), em(7), buf, GL_BLACK);

}

void draw_back_block(unsigned int selected)
{
    color_t COLOR = (selected == Back) ? GL_YELLOW : GL_WHITE;
    gl_draw_rect(em(5), em(5), em(20), em(11), COLOR);
    gl_draw_string(em(5), em(7), "Back", GL_BLACK);
}

void draw_matt_block(unsigned int selected, unsigned int selected_candidate)
{
    color_t COLOR;
    if (selected_candidate == Candidate1) {
        COLOR = (selected == Candidate1) ? GL_AMBER: 0xFF999999;
    } else {
        COLOR = (selected == Candidate1) ? GL_YELLOW : GL_WHITE;
    }
    gl_draw_rect(em(5), em(20), em(50), em(40), COLOR);
    gl_draw_string(em(10), em(33), "Matthew", GL_BLACK);
}

void draw_christos_block(unsigned int selected, unsigned int selected_candidate)
{
    color_t COLOR;
    if (selected_candidate == Candidate2) {
        COLOR = (selected == Candidate2) ? GL_AMBER: 0xFF999999;
    } else {
        COLOR = (selected == Candidate2) ? GL_YELLOW : GL_WHITE;
    }
    gl_draw_rect(em(65), em(20), em(50), em(40), COLOR);
    gl_draw_string(em(75), em(33), "Christos", GL_BLACK);
}

void draw_submit_vote_block(unsigned int selected, unsigned int selected_candidate)
{
    color_t COLOR;
    if (selected == SubmitBox) {
        COLOR = (selected_candidate == None) ? 0xFF444444 : GL_YELLOW;
    } else {
        COLOR = (selected_candidate == None) ? 0xFF999999 : GL_WHITE;
    }
    gl_draw_rect(em(5), em(65), em(100), em(25), COLOR);
    gl_draw_string(em(30), em(70), "Submit Vote", GL_BLACK);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


// --------------------- HOME PAGE ----------------------
void draw_admin_block(unsigned int selected) {
    color_t COLOR = (selected == AdminBox) ? GL_YELLOW : GL_WHITE;
    gl_draw_rect(em(5), em(5), em(50), em(40), COLOR);
    gl_draw_string(em(5), em(5), "Admin", GL_BLACK);
}

void draw_voting_block(unsigned int selected) {
    color_t COLOR = (selected == VoteBox) ? GL_YELLOW : GL_WHITE;
    gl_draw_rect(em(65), em(5), em(50), em(40), COLOR);
    gl_draw_string(em(65), em(5), "Vote", GL_BLACK);
}

void draw_fraud_proof_block(unsigned int selected) {
    color_t COLOR = (selected == FraudProofBox) ? GL_YELLOW : GL_WHITE;
    gl_draw_rect(em(5), em(55), em(50), em(40), COLOR);
    gl_draw_string(em(5), em(55), "Fraud Proof", GL_BLACK);
}

void draw_results_block(void) {
    color_t COLOR = (selected == SelectResultsBox) ? GL_YELLOW : GL_WHITE;
    gl_draw_rect(em(65), em(55), em(50), em(40), COLOR);
    gl_draw_string(em(65), em(55), "Results", GL_BLACK);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// --------------------- CERTIFICATE PAGE ----------------------
void draw_certificate_block(char* cert) {
    color_t COLOR = (selected == FraudProofBox) ? GL_YELLOW : GL_WHITE;
    gl_draw_rect(em(5), em(5), em(100), em(40), COLOR);
    gl_draw_string(em(10), em(10), cert, GL_BLACK);
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void move(unsigned int direction) {
    unsigned int next;
    switch (direction) {
        case 0: 
            next = key_map[selected].left;
            break;
        case 1:
            next = key_map[selected].right;
            break;
        case 2:
            next = key_map[selected].up;
            break;
        default:
            next = key_map[selected].down;
            break;
    }
    if (next == None) return;
    selected = next;
}
