//
// Contains the code to control each screen
//
#include "merkle.h"

// Define the screens
enum SelectedBox {
    None,
    Back,
    Candidate1,
    Candidate2,
    VoteBox,
    AdminBox,
    FraudProofBox,
    SelectResultsBox,
    SubmitBox,
    CertificateBox,
    MerkleBox,
    ResultsBox,
};

enum Screen {
    Home, 
    Auth,
    Vote,
    Certificate,
    Admin,
    FraudProof,
    Merkle,
    Results
};


void screen_init(void);

unsigned int get_selected(void);
void set_selected(unsigned int new_selected);

unsigned int get_selected_candidate(void);
void set_selected_candidate(unsigned int new_candidate);

unsigned int get_selected_screen(void);
void set_selected_screen(unsigned int new_screen);

//
// SCREEN DRAW FUNCTIONS
// 

void draw_vote_screen(void);
void draw_auth_screen(char* curr_pass);
void draw_home_screen(void);
void draw_cert_screen(char* cert);
void draw_fraud_proof_screen(char* cert);
void draw_fraud_visual_screen(node* merkle_proof, vote_merkle* merkle, int node_index);
void draw_results_screen(unsigned int num_votes, vote_merkle* merkle_tree);

void double_clear(void);
void switch_screen(unsigned int next_screen, unsigned int next_box);

// Block Draw Functions
void draw_title_block(void);
void draw_back_block(unsigned int selected);

void draw_matt_block(unsigned int selected, unsigned int selected_candidate);
void draw_christos_block(unsigned int selected, unsigned int selected_candidate);
void draw_submit_vote_block(unsigned int selected, unsigned int selected_candidate);

void draw_results_block(void);
void draw_admin_block(unsigned int selected);
void draw_voting_block(unsigned int selected);
void draw_fraud_proof_block(unsigned int selected);
void draw_certificate_block(char *);

typedef struct {
    unsigned int left;
    unsigned int right;
    unsigned int up;
    unsigned int down;
} key_mapping;
