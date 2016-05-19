/* File:     usfsort.c
 *
 * Purpose:  Sort the contents of a file and put into a new file
 * 
 * Input:    in - file name to sort
 * Output:   out - file name sorted
 *
 * Compile:  gcc -g -Wall -o usfsort usfsort.c
 * Run:      usfsort foo bar
 *
 * Requ:     Implement a doubly-linked list to store and sort the input lines
 *    done   Read each line into a linked list node
 *    done   Sort the linked list of lines (you can use an ordered insert for each line)
 *    done   Use malloc() to allocate linked list nodes and memory for each line (a string)
 *    done   You can assume a maximum length for each line
 *    done   Create <output_file> if it does not exist
 *    done   Overwrite <output_file> if it does exists
 *    done   Use system calls, not library calls (open(), close(), read(), write())
 *           Your solution should compile and run on Ubuntu Linux and Raspbian Linux (Optionally, you can support Mac OS X)
 *           Your code should be well formatted and commented
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

const int max = 1024;

struct list_node {
    char* word;
    struct list_node* next;
    struct list_node* prev;
};

struct list {
    struct list_node* head; 
    struct list_node* tail;
};

struct list_node* CreateNode(char* val);
struct list* CreateList();
void Print(struct list* list);
void Insert(struct list* list, char string[]);
void Write(int out, struct list* list, int max);

int main(int argc, char* argv[]) {

    int in, out;
    int count;
    char buf[max];
    char splitWord[max];
    struct list* list = CreateList();
    int j = 0;
    int i;
    int breaker = 0;
    
    in = open(argv[1], O_RDONLY);
    if (in < 0) {
        printf("Cannot open %s\n", argv[1]);
        exit(1);
    }
    
    out = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (in < 0) {
        printf("Cannot open %s\n", argv[2]);
        exit(1);
    }
    
    while ((count = read(in, buf, max - 1)) > 0) {
        for (i = 0; i < max - 1; i ++) {
            splitWord[j] = buf[i];
            j ++;
            // dog\ncat\nbanana\napple
            if (buf[i] == '\n' || buf[i] == -1) {
                splitWord[j - 1] = '\0';
                j = 0;
                Insert(list, splitWord);
                if (buf[i] == -1){
                    breaker = 1;
                    break;
                }
            }                     
        }
        if (breaker == 1){
            break;
        }
    }
    
    if (count == 0) {
      for (i = 0; i < max - 1; i ++) {
            splitWord[j] = buf[i];
            j ++;
            if (buf[i] == '\n') {
                splitWord[j] = '\0';
                j = 0;
                Insert(list, splitWord);
                break;
            }                       
        }
    }

    Write(out, list, max);
    
    Print(list);
    close(in);
    close(out);

    return 0;
}

/*-----------------------------------------------------------------
 * Function:    CreateNode
 * Purpose:     Create a new node with word, next and prev from file
 * Input args:  val:  word to put in the node's word space
 *              
 */
struct list_node* CreateNode(char* val) {
    
    struct list_node* newNode;

    newNode = (struct list_node *) malloc(sizeof(struct list_node));

    if (newNode == NULL) {
        printf("Couldn't create the node.\n");
        exit(1);
    }

    newNode -> word = malloc(strlen(val) * sizeof(char));
    newNode -> next = NULL;
    newNode -> prev = NULL;
    strncpy(newNode -> word, val, strlen(val));

    return newNode;

}  /* CreateNode */

/*-----------------------------------------------------------------
 * Function:    CreateList
 * Purpose:     Initialize an empty list and malloc space
 *
 */
struct list* CreateList() {
    struct list* newList;

    newList = (struct list*) malloc(sizeof(struct list));
    newList -> head = newList -> tail = NULL;

    if (newList == NULL) {
        printf("Couldn't create the list.\n");
        exit(1);
    }

    newList -> head = NULL;
    newList -> tail = NULL;

    return newList;

}  /* CreateList */

/*-----------------------------------------------------------------
 * Function:    Insert
 * Purpose:     Insert an item into a list
 * Input args:  list to put the newNode into
 *              string is the word that's being put into the node
 */
void Insert(struct list* list, char string[]) {

   struct list_node* curr = list -> head;
   struct list_node* temp;

    while (curr != NULL){
        if (strcmp(string, curr -> word) == 0) {
            return;  
        } else if (strcmp(string, curr -> word) < 0) {
            break;
        } else {
            curr = curr -> next;
        }
    }

   temp = CreateNode(string);
   strncpy(temp -> word, string, strlen(string));

   // empty list
    if (list -> head == NULL) {
        list -> head = list -> tail = temp;
    } 
    // one item in list goes after
    else if (curr == NULL) {
        temp -> prev = list -> tail;
        list -> tail -> next = temp;
        list -> tail = temp;
    } 
    // one item in list goes before
    else if (curr == list -> head) {
        temp -> next = list -> head;
        list -> head -> prev = temp;
        list -> head = temp;
    } 
    // more than 1 item in list put between two nodes
    else {
        temp -> next = curr;
        temp -> prev = curr -> prev;
        curr -> prev = temp;
        temp -> prev -> next = temp;
    }
}  /* Insert */

/*-----------------------------------------------------------------
 * Function:    Print
 * Purpose:     walk through the list and print entire contents
 * Input args:  list is the list to walk through the list
 */
void Print(struct list *list) {

    struct list_node *node;

    node = list -> head;
    while (node != NULL) {
        printf("[word] %-20s \n", node -> word);
        node = node -> next;
    }

}  /* Print */

/*-----------------------------------------------------------------
 * Function:    Write
 * Purpose:     walk through the list and write contents of list
 *              to the given file if any
 * Input args:  list is the list to walk through the list
 *              out is the output file
 *              max is the max list size
 */
void Write(int out, struct list* list, int max) {

    struct list_node *node;

    node = list -> head;
    while (node != NULL) {
        strcat(node -> word, "\n");
        write(out, node -> word, strlen(node -> word));
        node = node -> next;
    }

}  /* Write */