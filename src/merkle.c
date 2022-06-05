#include "merkle.h"
#include "strings.h"
#include "sha256.h"
#include "malloc.h"
#include "printf.h"
#include <stdbool.h>

/*
 * Implementation of the Merkel Vote Tree
 * Each leaf contains: Hash of children | Vote Accumulator.
 */

#define hexchar(x) x <= 9 ? '0' + x : 'a' + (x - 10)

void SHA256(const char * data, size_t len, char * hash) {
	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, (const BYTE *) data, len);
	sha256_final(&ctx, (BYTE *) hash);
}

// Turn a leaf into a node
void leaf_to_node(leaf * leafsrc, node * newnode) {
    SHA256((const char*) leafsrc, LEAF_SIZE, newnode->hash);
    newnode->vote_count = leafsrc->vote;
}

void bytes_to_hex(char * bytes, char * buf, int n) {
    size_t iter = 0;
    size_t buf_iter = 0;
    while (n--) {
        char first_byte = bytes[iter] >> 4;
        char second_byte = bytes[iter] & 0b1111;
        buf[buf_iter++] = hexchar(first_byte);
        buf[buf_iter++] = hexchar(second_byte);
        iter++;
    }
    buf[buf_iter] = '\0';
}

// Convert hex to bytes
void hex_to_bytes(const char * hex, char * buf, size_t n) {
    for (int i = 0; i < n; i += 2) {
        unsigned char first_hex = hex[i] >= 'a' ? 10 + (hex[i] - 'a') : hex[i] - '0';
        unsigned char second_hex = hex[i + 1] >= 'a' ? 10 + (hex[i + 1] - 'a') : hex[i + 1] - '0';
        buf[i / 2] = (first_hex << 4) + second_hex;
    }
}

void concat_nodes(node *left, node *right, char* buf, int buf_size) { memcpy(buf, left, NODE_SIZE);
    memcpy(&buf[NODE_SIZE], right, NODE_SIZE);
    buf[NODE_SIZE* 2] = '\0';
}

void hash_nodes(node *left, node *right, char* hash) { 
    char buf[NODE_SIZE * 2 + 1];
    concat_nodes(left, right, buf, NODE_SIZE * 2 + 1); 
    SHA256((const char *) buf, NODE_SIZE * 2, hash);
}

void combine_nodes(node *left, node *right, node* parent) {
    hash_nodes(left, right, parent->hash);
    parent->vote_count = left->vote_count + right->vote_count;
}

#define left_child(i) 2 * i + 1
#define right_child(i) 2 * i + 2
#define parent(i) (i - 1) / 2
#define is_right(i) i % 2 == 0

// Used to fill spots in leaf of merkle tree
const node EMPTY_NODE = {
    "00000000000000000000000000000000",
    0
};

vote_merkle* create_merkle_tree(leaf* leafs, int num_leafs) {
    // get depth of merkle tree
    unsigned int height = 0;
    unsigned int total_leafs = 1;
    while(total_leafs < num_leafs) {
        total_leafs *= 2;
        height += 1;
    }

    // initiate merkle tree
    vote_merkle* merkle = malloc(MERKLE_SIZE);
    merkle->height = height;
    merkle->nodes = malloc(NODE_SIZE * (total_leafs * 2  - 1));

    // populate bottom row
    node* bottom_nodes = &merkle->nodes[total_leafs - 1];
    for (size_t i = 0; i < num_leafs; i++) {
        leaf_to_node(&leafs[i], &bottom_nodes[i]);
    }
    for (size_t i = num_leafs; i < total_leafs; i++) {
        memcpy(&bottom_nodes[i], &EMPTY_NODE, NODE_SIZE);
    }

    // populate everything else 
    for (int i = total_leafs - 2; i >= 0; i--) {
        combine_nodes(&merkle->nodes[left_child(i)], &merkle->nodes[right_child(i)], &merkle->nodes[i]);
    }

    return merkle;
}

void print_bytes(void * bytes, int len) {
    char hash_str[len * 2 + 1];
    bytes_to_hex((char *) bytes, hash_str, len);
    printf("%s\n", hash_str);
}

node* create_merkle_proof(vote_merkle * merkle, size_t leaf_index) {
    node *merkle_proof = malloc(NODE_SIZE * merkle->height);
    int total_leafs = 1 << merkle->height;
    size_t index = leaf_index + total_leafs - 1;

    int curr_depth = 0;
    while (curr_depth < merkle->height) {
        int xparent = parent(index);
        int xneighbour = (is_right(index)) ? left_child(xparent): right_child(xparent);
        memcpy(&merkle_proof[curr_depth], &merkle->nodes[xneighbour], NODE_SIZE);
        index = xparent;
        curr_depth++;
    }

    return merkle_proof;
}

bool cmp(char* left, char* right, size_t n) {
    while (n--) {
        if (*left++ != *right++) {
            return false;
        }
    }
    return true;
}

bool verify_merkle_proof(node* merkle_root, node* merkle_proof, node* leaf_node, size_t leaf_index, size_t height) { 
    size_t total_leafs = 1 << height;
    int xindex = leaf_index + total_leafs - 1;
    node* curr_aggr = leaf_node;
    for (int i = 0; i < height; i++) {
        if (is_right(xindex)) {
            combine_nodes(&merkle_proof[i], curr_aggr, curr_aggr);
        } else {
            combine_nodes(curr_aggr, &merkle_proof[i], curr_aggr);
        }
        xindex = parent(xindex);
    }
    
    return cmp((char *) curr_aggr, (char *) merkle_root, NODE_SIZE);
}
