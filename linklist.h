#ifndef __LINKLIST_H__
#define __LINKLIST_H__ 

#ifndef NULL
#define NULL ((void *)0) 
#endif

/* A generic return value */
#ifndef ERROR
#define ERROR (-1)
#endif

#define __LIST_MAX 65535

/* Calculate the offset of a member in a struct */
#define offset_of(container, field) \
        ((unsigned long)&(((container *)0)->field))

/* Return the actual pointer that points to a list entry */
#define list_entry(lptr, container, field) \
        ((container *)((char *)lptr - offset_of(container, field)))

/* Iterating through the list */
#define list_for_each(itr, head) \
        for((itr) = (head)->next; (itr) != (head); (itr) = (itr)->next)

/* Iterating through the list, safe against element removal */
#define list_for_each_safe(itr, nx,  head) \
        for((itr) = (head)->next, (nx) = (itr)->next; (itr) != (head); (itr) = (nx), (nx) = (itr)->next)

/* Iterating through the list, reversed */
#define list_for_each_rev(itr, head) \
        for((itr) = (head)->prev; (itr) != (head); (itr) = (itr)->prev)

/* Iterating through the list, reversed , safe against element removal*/
#define list_for_each_rev_safe(itr, pv, head) \
        for((itr) = (head)->prev, (pv) = (itr)->prev; (itr) != (head); (itr) = (pv), (pv) = (itr)->next)

/* Iterating through the specific range */
/* itr MUST be set to the starting point of the iteration, it fails if itr == head */
#define list_for_range(itr, i, count, head) \
        for((i) = 0; (i<(count))&&((itr) != (head)); i++, (itr) = (itr)->next)

/* Iterating through the specific range, safe against element removal */
/* itr MUST be set to the starting point of the iteration, it fails if itr == head */
#define list_for_range_safe(itr, nx, i, count, head) \
        for((i) = 0, (nx) = (itr)->next; (i<(count))&&((itr) != (head)); i++, (itr) = (nx), (nx) = (itr)->next)

/* Iterating through the specific range, reversed */
/* itr MUST be set to the starting point of the iteration, it fails if itr == head */
#define list_for_range_rev(itr, i, count, head) \
        for((i) = count-1; (i>=0)&&((itr) != (head)); i--, (itr) = (itr)->prev)

/* Iterating through the specific range, reversed, safe against element removal */
/* itr MUST be set to the starting point of the iteration, it fails if itr == head */
#define list_for_range_rev_safe(itr, pv, i, count, head) \
        for((i) = count-1, (pv) = (itr)->prev; (i>=0)&&((itr) != (head)); i--, (itr) = (pv), (pv) = (itr)->prev)

/* internal insert method, prev should be right before next */
#define __list_ins(e, pv, nx) \
        ({ (e)->next = (nx); (e)->prev = (pv); \
           (pv)->next = (e); (nx)->prev = (e); })

/* Insert an element, count specifies the position. */
/* if count is larger than the length of the list, e is inserted in the head */
#define list_ins(head, e, count) \
        ({ int i; list_node *itr = (head)->next, *pv; \
           list_for_range(itr, i, count, head); \
           pv = itr->prev; \
           __list_ins(e, pv, itr); })

/* Insert an element at the head of the list */
#define list_ins_head(head, e) \
        ({ list_node *nx = (head)->next; __list_ins(e, head, nx); })

/* Insert an element at the tail of the list */
#define list_ins_tail(head, e) \
        ({ list_node *pv = (head)->prev; __list_ins(e, pv, head); })

/* internal delete method */
#define __list_del(e, pv, nx) \
        ({ (pv)->next = (nx); (nx)->prev = (pv); \
           (e)->next = (e)->prev = (e); })

/* Delete an element, count specifies which to delete. */
/* if count is larger than the length of the list, nothing happens and head is returned */
#define list_del_n(head, count) \
        ({ int i; list_node* itr = (head)->next; \
           list_for_range(itr, i, count, head); \
           list_del(head, itr); })

/* Simply delete the element e */
#define list_del(head, e) \
        ({ if((e) != (head)) __list_del(e, (e)->prev, (e)->next); e; })

/* Delete an element from the head side */
#define list_del_head(head) \
        ({ list_node *nx = (head)->next; list_del(head, nx); })

/* Delete an element from the tail side */
#define list_del_tail(head) \
        ({ list_node *pv = (head)->prev; list_del(head, pv); })

/* Find out the length of the list */
#define list_len(head) \
        ({ int len; list_node* itr = (head)->next; \
           list_for_range(itr, len, __LIST_MAX, head); \
           len; })

/* true if list is empty */
#define list_empty(head) \
        (((head)->next == (head)) && ((head)->prev == (head)))

/* Iterate through and call free() */
#define list_free(head, container, field) \
        ({ list_node* itr, *nx; \
           list_for_each_safe(itr, nx, head){ \
               free(list_entry(itr, container, field)); \
           } \
           (head)->next = (head)->prev = (head); })

/* type for a list node */
typedef struct __list_node list_node;
struct __list_node{
    list_node *prev, *next;
};

#endif

