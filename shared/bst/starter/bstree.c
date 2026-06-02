/*******************************************************************************
 * Name        : bstree.c
 * Author      : William Ee
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include "bstree.h"

static node_t* make_node(void* data, size_t bytes) {
    node_t* n = (node_t*)malloc(sizeof(node_t));

    if (n == NULL) {
        return NULL;
    }

    n->left = NULL;
    n->right = NULL;

    n->data = malloc(bytes);

    if (n->data == NULL) {
        free(n);
        
        return NULL;
    }

    char* dst = (char*)n->data;
    char* src = (char*)data;

    for (size_t i = 0; i < bytes; i++) {
        dst[i] = src[i];
    }

    return n;
}

static void insert_node(node_t* curr, node_t* new, int (*cmpr)(void*, void*)) {
    if (cmpr(new->data, curr->data) < 0) {
        if (curr->left == NULL) {
            curr->left = new;
        } else {
            insert_node(curr->left, new, cmpr);
        }
    } else {
        if (curr->right == NULL) {
            curr->right = new;
        } else {
            insert_node(curr->right, new, cmpr);
        }
    }
}

void add_node(void* data, size_t bytes, tree_t* tree, int (*cmpr)(void*, void*)) {
    if (data == NULL || bytes == 0 || tree == NULL || cmpr == NULL) {
        return;
    }

    node_t* n = make_node(data, bytes);

    if (n == NULL) {
        return;
    }

    if (tree->root == NULL) {
        tree->root = n;
        
        return;
    }

    insert_node(tree->root, n, cmpr);
}

void print_tree(node_t* root, void (*print_fn)(void*)) {
    if (root == NULL || print_fn == NULL) {
        return;
    }

    print_tree(root->left, print_fn);
    print_fn(root->data);
    print_tree(root->right, print_fn);
}

static void destroy_help(node_t* root) {
    if (root == NULL) {
        return;
    }

    destroy_help(root->left);
    destroy_help(root->right);

    free(root->data);
    free(root);
}

void destroy(tree_t* tree) {
    if (tree == NULL) {
        return;
    }

    destroy_help(tree->root);
    tree->root = NULL;
}
