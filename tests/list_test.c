/*
 * =====================================================================================
 *
 *       Filename:  list_test_gen.c
 *
 *    Description:  A more formal test for linklist.h
 *
 *        Version:  1.0
 *        Created:  10/02/2008 04:13:16 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  l_amee (l_amee), l04m33@gmail.com
 *        Company:  SYSU
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>

#include "linklist.h"

struct tdata{
    int d;
    list_node list;
};

static list_node head = { &head, &head };

void handle_get(char ***ptr){
    char **tmp = *ptr;
    int idx = atoi(tmp[1]);

    list_node *itr = head.next; int i;
    list_for_range(itr, i, idx, &head);
    struct tdata *data;
    if(itr == &head)
        printf("%d:head\n", idx);
    else{
        data = list_entry(itr, struct tdata, list);
        printf("%d:%d\n", idx, data->d);
    }
    (*ptr) += 2;
}

void handle_ins(char ***ptr){
    char **tmp = *ptr;
    int idx = atoi(tmp[2]);
    int val = atoi(tmp[1]);

    struct tdata *data = (struct tdata*)malloc(sizeof(struct tdata));
    data->d = val;
    list_ins(&head, &data->list, idx);

    (*ptr) += 3;
}

void handle_ins_side(char ***ptr){
    char **tmp = *ptr;
    int val = atoi(tmp[1]);

    struct tdata *data = (struct tdata*)malloc(sizeof(struct tdata));
    data->d = val;
    if(tmp[0][0] == 'h')
        list_ins_head(&head, &data->list);
    else if(tmp[0][0] == 't')
        list_ins_tail(&head, &data->list);

    (*ptr) += 2;
}

void handle_del(char ***ptr){
    char **tmp = *ptr;
    int idx = atoi(tmp[1]);

    list_node *lp = list_del_n(&head, idx);
    if(lp != &head)
        free(list_entry(lp, struct tdata, list));

    (*ptr) += 2;
}

void handle_del_side(char ***ptr){
    char **tmp = *ptr;
    list_node *lp;
    
    if(tmp[0][0] == 'q')
        lp = list_del_tail(&head);
    else if(tmp[0][0] == 'p')
        lp = list_del_head(&head);
    if(lp != &head)
        free(list_entry(lp, struct tdata, list));

    *ptr += 1;
}

void handle_len(char ***ptr){
    char **tmp = *ptr;

    printf("l:%d\n", list_len(&head));

    (*ptr) += 1;
}

void handle_empty(char ***ptr){
    char **tmp = *ptr;

    printf("e:%d\n", list_empty(&head));

    (*ptr) += 1;
}

int main(int argc, char **argv)
{
    int i;
    char **ptr = argv + 1;
    for( ; ptr < argv + argc; i++){
        switch(ptr[0][0]){
            case 'g': handle_get(&ptr); break;
            case 'i': handle_ins(&ptr); break;
            case 'h': handle_ins_side(&ptr); break;
            case 't': handle_ins_side(&ptr); break;
            case 'd': handle_del(&ptr); break;
            case 'p': handle_del_side(&ptr); break;
            case 'q': handle_del_side(&ptr); break;
            case 'l': handle_len(&ptr); break;
            case 'e': handle_empty(&ptr); break;
            default: break;
        }
    }

    return 0;
}
