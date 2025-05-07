#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    int key;
    int blinkPeriod;
    char ledcolor;
    struct node *left;
    struct node *right;
};

struct node *root = NULL;

struct node *new_node(int key, int blinkPeriod, char ledcolor) {
    struct node *temp = (struct node *)malloc(sizeof(struct node));
    temp->key = key;
    temp->blinkPeriod = blinkPeriod;
    temp->ledcolor = ledcolor;
    temp->left = temp->right = NULL;
    return temp;
}

struct node *insert(struct node *root, int key, int blinkPeriod, char ledcolor) {
    if (root == NULL) {
        return new_node(key, blinkPeriod, ledcolor);
    }
    if (key < root->key) {
        root->left = insert(root->left, key, blinkPeriod, ledcolor);
    } else {
        root->right = insert(root->right, key, blinkPeriod, ledcolor);
    }
    return root;
}

struct node *find_minimum(struct node *root) {
    while (root->left != NULL) {
        root = root->left;
    }
    return root;
}

struct node *delete_node(struct node *root, int key) {
    if (root == NULL) {
        return root;
    }
    if (key < root->key) {
        root->left = delete_node(root->left, key);
    } else if (key > root->key) {
        root->right = delete_node(root->right, key);
    } else {
        if (root->left == NULL) {
            struct node *temp = root->right;
            free(root);
            return temp;
        } else if (root->right == NULL) {
            struct node *temp = root->left;
            free(root);
            return temp;
        }
        struct node *temp = find_minimum(root->right);
        root->key = temp->key;
        root->blinkPeriod = temp->blinkPeriod;
        root->ledcolor = temp->ledcolor;
        root->right = delete_node(root->right, temp->key);
    }
    return root;
}

void inorder(struct node *root) {
    if (root != NULL) {
        inorder(root->left);
        printf("%d\t%d\t%c\n", root->key, root->blinkPeriod, root->ledcolor);
        inorder(root->right);
    }
}

void readCSV(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file %s\n", filename);
        return;
    }

    char line[100];
    int key, blinkPeriod;
    char ledcolor;

    printf("Reading data from %s and inserting into the BST:\n", filename);
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%d,%d,%c", &key, &blinkPeriod, &ledcolor) == 3) {
            root = insert(root, key, blinkPeriod, ledcolor);
        }
    }
    fclose(file);
}

void printMenu() {
    printf("\n==== Menu ====\n");
    printf("1 - Insertion\n");
    printf("2 - Deletion\n");
    printf("3 - Display BST\n");
    printf("4 - Exit\n");
    printf("===============\n");
}

int main() {
    const char *filename = "blink.csv";
    
    // Read CSV and build the BST
    readCSV(filename);
    
    while (1) {
        int choice;
        printMenu();
        printf("Enter your choice: ");
        scanf("%d", &choice);

        int key, blinkPeriod;
        char ledcolor;

        switch (choice) {
        case 1:
            printf("Enter the node values to insert (key, blinkPeriod, ledcolor):\n");
            scanf("%d %d %c", &key, &blinkPeriod, &ledcolor);
            root = insert(root, key, blinkPeriod, ledcolor);
            printf("Updated BST:\n");
            inorder(root);
            break;

        case 2:
            printf("Enter the key value to delete:\n");
            scanf("%d", &key);
            root = delete_node(root, key);
            printf("Updated BST:\n");
            inorder(root);
            break;

        case 3:
            printf("Inorder Traversal of BST:\n");
            inorder(root);
            break;

        case 4:
            printf("Exiting program...\n");
            exit(0);

        default:
            printf("Invalid choice. Please enter a valid option.\n");
            break;
        }
    }

    return 0;
}
