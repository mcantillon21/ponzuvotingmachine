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


// int main()
// {
//     // unsigned char hash[100];
//     // const unsigned char *data = "123";
//     // SHA256(data, 3, hash);
//     // unsigned char hash_str[64];
//     // bytes_to_hex(hash, hash_str, 32);
//     // printf("%s\n", hash_str);

//     node left_node;
//     memcpy(&left_node, "0123456789abcdef0123456789abcdef", 32);
//     left_node.vote_count = 2;

//     node right_node;
//     memcpy(&right_node, "0123456789abcdef0123456789abcdef", 32);
//     right_node.vote_count = 1;

// 	SHA256_CTX ctx;

//     char hash[32];
//     SHA256("password", 8, hash);
//     print_bytes(hash, 32);

//     leaf leafs[3];
//     memset(&leafs[0], '\0', LEAF_SIZE);
//     memcpy(leafs[0].sig, "password", 32);
//     hex_to_bytes("5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", leafs[0].pk, 64);
//     memcpy(leafs[0].randomness, "123", 3);
//     leafs[0].vote = 1;

//     memset(&leafs[1], '\0', LEAF_SIZE);
//     memcpy(leafs[1].sig, "password", 32);
//     hex_to_bytes("5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", leafs[1].pk, 64);
//     memcpy(leafs[1].randomness, "123", 3);
//     leafs[1].vote = 0;
    
//     memset(&leafs[2], '\0', LEAF_SIZE);
//     memcpy(leafs[2].sig, "password", 32);
//     hex_to_bytes("5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", leafs[2].pk, 64);
//     memcpy(leafs[2].randomness, "123", 3);
//     leafs[2].vote = 2;

//     vote_merkle* merkle_tree = create_merkle_tree(leafs, 3);

//     printf("%d\n", (unsigned int) merkle_tree->height);
//     print_bytes(&merkle_tree->nodes[0], 33);
//     print_bytes(&merkle_tree->nodes[1], 33);
//     print_bytes(&merkle_tree->nodes[2], 33);
//     print_bytes(&merkle_tree->nodes[3], 33);

//     node* merkle_proof = create_merkle_proof(merkle_tree, 0);

//     print_bytes(&merkle_proof[0], 33);
//     print_bytes(&merkle_proof[1], 33);

//     return 0;
// }