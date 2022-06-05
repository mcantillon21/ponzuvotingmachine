#include <stdbool.h>
#include <stddef.h>

#define PTR_SIZE 8
#define NODE_SIZE 33
#define UNSIGNED_LEAF_SIZE 65
#define LEAF_SIZE 97 
#define MERKLE_SIZE 1 + PTR_SIZE

typedef struct {
    char hash[32];
    char vote_count;
} node;

typedef struct { 
    char hash[32]; // Public Key of voter
    char randomness[32]; // Randomness 
    char vote; // 0 or 1 
    char sig[32]; // Signature
} leaf;

typedef struct {
    unsigned char height;
    node * nodes;
} vote_merkle;

void SHA256(const char * data, size_t len, char * hash);

void print_bytes(void * bytes, int len);
void leaf_to_node(leaf * leafsrc, node * newnode);

void bytes_to_hex(char * bytes, char * buf, int n);

void hex_to_bytes(const char * hex, char * buf, size_t n);

void concat_nodes(node *left, node *right, char* buf, int buf_size);

void hash_nodes(node *left, node *right, char* hash);

void combine_nodes(node *left, node *right, node* parent);

vote_merkle* create_merkle_tree(leaf* leafs, int num_leafs);

node* create_merkle_proof(vote_merkle * merkle, size_t leaf_index);

bool cmp(char * left, char * right, size_t n);

bool verify_merkle_proof(node* merkle_root, node* merkle_proof, node* leaf_node, size_t leaf_index, size_t height);