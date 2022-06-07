#include "gl.h"
#include "screen.h"
#include "strings.h"
#include "printf.h"

static unsigned int selected = AdminBox;
static unsigned int selected_screen = Home;
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
    gl_draw_rect(em(24), em(29), em(55), em(6), 0xF7DCB4);

    gl_draw_string(em(20), em(45), pass_error, GL_RED);

    gl_draw_rect(em(15), em(70), em(90), em(25), 0xF7DCB4);
    gl_draw_string(em(37), em(80), "Press Enter", GL_BLACK);
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
    gl_draw_string(em(20), em(15), "Enter Password:", GL_BLACK);
    gl_draw_rect(em(24), em(29), em(35), em(6), 0xF7DCB4);
    gl_draw_string(em(25), em(30), display, GL_BLACK);

    gl_draw_rect(em(15), em(60), em(90), em(25), GL_WHITE);
    gl_draw_string(em(40), em(70), "Press Enter", GL_BLACK);
}

void draw_admin_screen(char * voter_name, char * curr_pass_input, char * success) {
    gl_draw_rect(em(15), em(10), em(90), em(70), GL_WHITE);
    gl_draw_string(em(20), em(20), "Enter Name:", GL_BLACK);
    gl_draw_rect(em(24), em(29), em(35), em(6), 0xF7DCB4);
    gl_draw_rect(em(24), em(49), em(65), em(6), 0xF7DCB4);
    gl_draw_string(em(25), em(30), voter_name, GL_BLACK);
    gl_draw_string(em(20), em(40), "Enter New Vote Phrase:", GL_BLACK);

    gl_draw_string(em(25), em(60), success, 0x4BB543);
}

void draw_fraud_proof_screen(char * cert) {
    gl_draw_rect(em(5), em(5), em(110), em(40), GL_WHITE);
    gl_draw_string(em(10), em(10), "Enter Certificate:", GL_BLACK);
    gl_draw_string(em(15), em(20), cert, GL_BLACK);

    gl_draw_rect(em(5), em(60), em(110), em(25), GL_WHITE);
    gl_draw_string(em(40), em(70), "Press Enter", GL_BLACK);
}

void merkle_rect(int x, int y, color_t color, unsigned int num, bool show_num) {
    gl_draw_rect(x - em(3), y - em(3), em(6), em(6), color);
    if (show_num) {
        char num_buffer[3];
        unsigned_to_base(num_buffer, 3, num, 10, 1);
        gl_draw_string(x - em(3) + em(1), y - em(3) + em(1), num_buffer, GL_BLACK);
    }
 }

#define left_child(i) 2 * i + 1
#define right_child(i) 2 * i + 2
#define parent(i) (i - 1) / 2
#define is_right(i) i % 2 == 0

void draw_fraud_visual_screen(node* merkle_proof, vote_merkle* merkle, int node_index, bool empty_proof) {
    if (node_index == -1) {
        gl_draw_rect(em(5), em(5), em(110), em(90), GL_WHITE);
        gl_draw_string(em(10), em(10), "Invalid Certificate", GL_BLACK);

        gl_draw_string(em(10), em(20), "Press Enter to Return", GL_BLACK);
        gl_draw_string(em(10), em(30), "Home", GL_BLACK);

     } else {
        gl_draw_rect(em(5), em(5), em(110), em(90), GL_WHITE);

        merkle_rect(em(20), em(76), GL_RED, 0, false);
        merkle_rect(em(20), em(83), 0xFF4281f5, 0, false);

        if (empty_proof) {
            gl_draw_string(em(10), em(10), "Empty Tree Proof:", GL_BLACK);
            gl_draw_string(em(30), em(74), "Merkle Proof Nodes", GL_BLACK);
            gl_draw_string(em(30), em(81), "'Empty Path'", GL_BLACK);
        } else {
            gl_draw_string(em(10), em(10), "Valid Certificate:", GL_BLACK);
            gl_draw_string(em(30), em(74), "Merkle Proof Nodes", GL_BLACK);
            gl_draw_string(em(30), em(81), "Your Path", GL_BLACK);
        }

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
                if (color == 0) {
                    merkle_rect(currx, curr_y, color, 0, false);
                } else {
                    merkle_rect(currx, curr_y, color, (unsigned int) merkle->nodes[true_index].vote_count, true);
                }
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

    gl_draw_string(em(10), em(10), "Matt Votes:", GL_BLACK);
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
    gl_draw_rect(em(30), em(5), em(85), em(11), COLOR);
    char buf[100];
    snprintf(buf, 100,  "Welcome, %s", name);
    gl_draw_string(em(35), em(8), buf, GL_BLACK);

}

void draw_back_block(unsigned int selected)
{
    color_t COLOR = (selected == Back) ? 0xF7DCB4 : GL_WHITE;
    gl_draw_rect(em(5), em(5), em(20), em(11), COLOR);
    gl_draw_string(em(8), em(8), "Back", GL_BLACK);
}


void draw_christos_block(unsigned int selected, unsigned int selected_candidate)
{
    color_t COLOR;
    if (selected_candidate == Candidate2) {
        COLOR = (selected == Candidate2) ? GL_AMBER: 0xFF999999;
    } else {
        COLOR = (selected == Candidate2) ? 0xF7DCB4 : GL_WHITE;
    }
    gl_draw_rect(em(63), em(20), em(52), em(40), COLOR);
    gl_draw_string(em(75), em(35), "Christos", GL_BLACK);
}

void draw_submit_vote_block(unsigned int selected, unsigned int selected_candidate)
{
    color_t COLOR;
    if (selected == SubmitBox) {
        COLOR = (selected_candidate == None) ? 0xFF444444 : GL_AMBER;
    } else {
        COLOR = (selected_candidate == None) ? 0xFF999999 : GL_WHITE;
    }
    gl_draw_rect(em(5), em(65), em(110), em(25), COLOR);
    gl_draw_string(em(40), em(75), "Submit Vote", GL_BLACK);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


// --------------------- HOME PAGE ----------------------
void draw_admin_block(unsigned int selected) {
    gl_draw_string(em(25), em(5), "PONZU VOTING MACHINE", GL_WHITE);
    color_t COLOR = (selected == AdminBox) ? 0xF7DCB4 : GL_WHITE;
    gl_draw_rect(em(8), em(15), em(50), em(30), COLOR);
    gl_draw_string(em(23), em(28), "Admin", GL_BLACK);
}

void draw_voting_block(unsigned int selected) {
    color_t COLOR = (selected == VoteBox) ? 0xF7DCB4 : GL_WHITE;
    gl_draw_rect(em(63), em(15), em(50), em(30), COLOR);
    gl_draw_string(em(80), em(28), "Vote", GL_BLACK);
}

void draw_fraud_proof_block(unsigned int selected) {
    color_t COLOR = (selected == FraudProofBox) ? 0xF7DCB4 : GL_WHITE;
    gl_draw_rect(em(8), em(50), em(50), em(30), COLOR);
    gl_draw_string(em(14), em(63), "Fraud Proof", GL_BLACK);
}

void draw_results_block(void) {
    color_t COLOR = (selected == SelectResultsBox) ? 0xF7DCB4 : GL_WHITE;
    gl_draw_rect(em(63), em(50), em(50), em(30), COLOR);
    gl_draw_string(em(75), em(63), "Results", GL_BLACK);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// --------------------- CERTIFICATE PAGE ----------------------
void draw_certificate_block(char* cert) {
    color_t COLOR = (selected == FraudProofBox) ? GL_YELLOW : GL_WHITE;
    gl_draw_rect(em(5), em(5), em(100), em(40), COLOR);
    gl_draw_string(em(10), em(10), "Confirmation Hash:", GL_BLACK);
    gl_draw_string(em(10), em(20), cert, GL_BLACK);
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct  {
    unsigned char first_char, last_char;
    size_t img_width, img_height;
    unsigned char pixel_data[];
} img_t;

static const img_t img_default;
static const img_t *g_img = &img_default;

size_t img_get_glyph_height(void) {
    return g_img->img_height;
}

size_t img_get_glyph_width(void) {
    return g_img->img_width;
}

size_t img_get_glyph_size(void) {
    return img_get_glyph_width() * img_get_glyph_height();
}


// static const img_t img_default = {
//     .img_width = 84, .img_height = 48,
//     .pixel_data = {
//     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x80, 0x0, 0x0, 0x7, 0xf0, 
//     0x0, 0x0, 0xd, 0xa0, 0x0, 0x0, 0x1c, 0xf0, 0x0, 0x0, 0x1f, 0x90, 0x0, 0x0, 0x7, 
//     0xe0, 0x0, 0x0, 0xe, 0x40, 0x0, 0x0, 0x1a, 0x70, 0x0, 0x0, 0x19, 0x98, 0x0, 0x0, 
//     0x1f, 0xf8, 0x0, 0x0, 0x6, 0x60, 0x0, 0x0, 0xe, 0x70, 0x0, 0x0, 0x0, 0x0, 0x0, 
//     0x0, 0x0, 0x0, 0x0 }
// };

// bool img_get_glyph(unsigned char buf[], size_t buflen) {
//     int index = 0;
//     // int nbits_in_row = (g_img->last_char - g_img->first_char + 1) * img_get_glyph_width();
//     int nbits_in_row = img_default.img_width;
//     int x_offset = 0;
//     for (int y = 0; y < img_default.img_height; y++) {
//         for (int x = 0; x < img_default.img_height; x++) {
//             int bit_index = y * nbits_in_row + x;
//             int bit_start = bit_index / 8;
//             int bit_offset = bit_index % 8;
//             // extract single bit for this pixel from bitmap
//             int val = img_default.pixel_data[bit_index] & (1 << (7 - bit_offset));
//             // use 0xff for on pixel, 0x0 for off pixel
//             buf[index++] = val != 0 ? 0xFF : 0x00;
//         }
//     }
//     return true;
// }

// void gl_draw_img(int x, int y, color_t c)
// {
//     // Load glyph to char_map.
//     unsigned int glyph_size = img_default.img_height * img_default.img_height;
//     unsigned char buf[glyph_size];
//     // if (!img_get_glyph(ch, buf, glyph_size)) return;
//     unsigned int image_width = img_default.img_width;
//     unsigned int image_height = img_default.img_height;
//     if (!img_get_glyph(buf, glyph_size)) return;
//     // img_get_glyph(ch, buf, glyph_size);
//     unsigned char (*char_map)[image_width] = (void *) buf;


//     // Copy char_map to frame buffer, with position x, y being upper left corner
//     // of the char_map.
//     for (int dy = 0; dy < image_height; dy++) {
//         for (int dx = 0; dx < image_width; dx++) {
//             if (char_map[dy][dx] == 0xff) gl_draw_pixel(x + dx, y + dy, c | 0xff000000);
//         }
//     }
// }

void draw_matt_block(unsigned int selected, unsigned int selected_candidate)
{
    // gl_draw_img(5, 5, GL_BLACK);
    color_t COLOR;
    if (selected_candidate == Candidate1) {
        COLOR = (selected == Candidate1) ? GL_AMBER: 0xFF999999;
    } else {
        COLOR = (selected == Candidate1) ? 0xF7DCB4 : GL_WHITE;
    }
    gl_draw_rect(em(5), em(20), em(52), em(40), COLOR);
    gl_draw_string(em(16), em(36), "Matthew", GL_BLACK);
}