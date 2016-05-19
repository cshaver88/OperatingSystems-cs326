#include "fcntl.h"
#include "types.h"
#include "stat.h"
#include "user.h"


#define NULL (void *) 0


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

//For gdb == /opt/i386dev/bin/i386-xv6-elf-gdb kernel
struct list_node* create_node(char* val);
struct list* create_list();
void print(struct list* list);
void insert(struct list* list, char string[]);
void write_to_file(int out, struct list* list, int max);

int main(int argc, char* argv[]) {

    int in, out;
    int count;
    char buf[max];
    char splitWord[max];
    struct list* list = create_list();
    int j = 0;
    int i;
    int breaker = 0;
    
    in = open(argv[1], O_RDONLY);
    if (in < 0) {
        printf(1, "Cannot open %s\n", argv[1]);
        exit();
    }
    //O_CREAT | O_TRUNC | O_WRONLY,
    out = open(argv[2], O_CREATE | O_WRONLY);
    if (in < 0) {
        printf(1, "Cannot open %s\n", argv[2]);
        exit();
    }
    
    while ((count = read(in, buf, max - 1)) > 0) {
        for (i = 0; i < max - 1; i ++) {
            splitWord[j] = buf[i];
            j ++;
            // dog\ncat\nbanana\napple
            if (buf[i] == '\n' || buf[i] == -1) {
                splitWord[j - 1] = '\0';
                j = 0;
                insert(list, splitWord);
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
                if (buf[i] != -1) {
                    splitWord[j] = '\0';
                    j = 0;
                    insert(list, splitWord);
                    break;
            }                       
        }
    }

    write_to_file(out, list, max);

    close(in);
    close(out);

    exit();
    return 0;
}

/*-----------------------------------------------------------------
 * Function:    CreateNode
 * Purpose:     Create a new node with word, next and prev from file
 * Input args:  val:  word to put in the node's word space
 *              
 */
struct list_node* create_node(char* val) {
    
    struct list_node* newNode;

    newNode = (struct list_node *) malloc(sizeof(struct list_node));

    if (newNode == NULL) {
        printf(1, "Couldn't create the node.\n");
        exit();
    }

    newNode -> word = malloc(strlen(val) * sizeof(char));
    newNode -> next = NULL;
    newNode -> prev = NULL;
    strcpy(newNode -> word, val);

    return newNode;

}  /* create_node */

/*-----------------------------------------------------------------
 * Function:    create_list
 * Purpose:     Initialize an empty list and malloc space
 *
 */
struct list* create_list() {
    struct list* newList;

    newList = (struct list*) malloc(sizeof(struct list));
    newList -> head = newList -> tail = NULL;

    if (newList == NULL) {
        printf(1, "Couldn't create the list.\n");
        exit();
    }

    newList -> head = NULL;
    newList -> tail = NULL;

    return newList;

}  /* create_list */

/*-----------------------------------------------------------------
 * Function:    insert
 * Purpose:     Insert an item into a list
 * Input args:  list to put the newNode into
 *              string is the word that's being put into the node
 */
void insert(struct list* list, char string[]) {

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

   temp = create_node(string);
   strcpy(temp -> word, string);

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
}  /* insert */

/*-----------------------------------------------------------------
 * Function:    print
 * Purpose:     walk through the list and print entire contents
 * Input args:  list is the list to walk through the list
 */
void print(struct list *list) {

    struct list_node *node;

    node = list -> head;
    while (node != NULL) {
        printf(1, "[word] %-20s \n", node -> word);
        node = node -> next;
    }

}  /* print */

/*-----------------------------------------------------------------
 * Function:    write
 * Purpose:     walk through the list and write contents of list
 *              to the given file if any
 * Input args:  list is the list to walk through the list
 *              out is the output file
 *              max is the max list size
 */
void write_to_file(int out, struct list* list, int max) {

    struct list_node *node;
    char* newWord;

    node = list -> head;
    while (node != NULL) {
        newWord = malloc(sizeof(node -> word) + 1);
        strcpy(newWord, node -> word);
        newWord[strlen(newWord)] = newWord[strlen(newWord) - 1];
        newWord[strlen(newWord) - 1] = '\n';
        strcpy(node -> word, newWord);
        write(out, node -> word, strlen(node -> word));
        node = node -> next;
    }

}  /* write */
