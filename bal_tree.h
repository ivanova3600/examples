#ifndef _BAL_TREE_H_
#define _BAL_TREE_H_
	
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct node{
	int key;
	struct node *left;
	struct node *right;
	unsigned char height;
};

typedef struct node Node;

Node *create(int key){
	Node *p = (Node*) malloc(sizeof(Node));
	p->key = key; 
	p->left = p->right = 0;
	p->height = 1;
	return p;
}

Node* insert(Node* tree, int val)
{
	if(!tree)
		return create(val);
	if(val < tree->key)
		tree->left=insert(tree->left, val);
	else
		tree->right=insert(tree->right, val);
	return(tree);
}


Node *min_value(Node *tree)
{
    if (tree->left) {
        return min_value(tree->left);
    } else {
        return(tree);
    }
}

Node* delete_node(Node* tree, int val)
{
	if (!tree) 
		return tree;
	else if(val<tree->key)
		tree->left=delete_node(tree->left, val);
	else if (val>tree->key)
		tree->right = delete_node(tree->right, val);
	else{
		Node *tmp=tree;
		if(tmp->left && tmp->right){ 
			tmp=min_value(tmp->right);
			tree->key=tmp->key; tree->right=delete_node(tree->right, tree->key);
		}
		else if(!(tmp->right)) tree=tmp->left; 
		else if(!(tmp->left))	tree=tmp->right; 
		free(tmp);
		}
	return(tree);
}

void delete_tree(Node *tree) {
  if(tree){
    delete_tree(tree->left);
    delete_tree(tree->right);
    free(tree);
  }
}

int find_key(Node *p){
	if(!p) return 0;
	if(!p->left && !p->right) return p->key;
	else if(p->right) return find_key(p->right);
	else return find_key(p->left);
}

Node *search(Node *p, int key){

	if(!p)
		return NULL;
	if(p->key > key)
		return search(p->left, key);
	else if(p->key < key)
		return search(p->right, key);
	return p;
}

void print(Node *p){
	if(p){
		print(p->left);
		printf("%d ", p->key);
		print(p->right);
	}
}

#endif