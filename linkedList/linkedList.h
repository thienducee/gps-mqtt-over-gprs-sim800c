/*******************************************************************************
 * Copyright (c) 2009, 2014 VNPT Technology Company.
 *
 * Contributors:
 *       Filename:  linkedList.h
 *
 *    Description:  the header file
 *
 *        Version:  1.0
 *        Created:  2017-6-18 15:58:20
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  Le Thien Duc, thienduc.ee@gmail.com - initial implementation
 *   Organization:  none
 *******************************************************************************/
#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

#include <stdint.h>
#include <string.h>
typedef struct _linkedList_Element_t {
  struct _linkedList_Element_t* next;
  void* content;
} linkedList_Element_t;

typedef struct {
  linkedList_Element_t* head;
  linkedList_Element_t* tail;
  uint32_t count;
} linkedList_t;

/** @brief linkedListInit
 *
 * This function will return an initialized and empty LinkedList object.  This
 * makes use of `malloc()` and can be freed when the `linkedListDeinit()`
 * function is called.
 * @return: A linkedList poiter
 */
linkedList_t* linkedList_Init(void);

/** @brief linkedListDeinit
 *
 * This function will empty and free a LinkedList object.
 *
 * @param list Pointer to the list to clear and free
 * @return none
 */
void linkedList_Deinit(linkedList_t* list);

/** @brief linkedListPushBack
 *
 * This function will push an element on to the back of a list.  This
 * makes use of `malloc()` and will call the corresponding `free()` the pop
 * function is called.
 *
 * @param list Pointer to the list to push the element to
 * @param content void pointer to an object or value to push to back of the list
 * @return none
 */
void linkedList_PushBack(linkedList_t* list, void* content);

/** @brief linkedListPopFront
 *
 * This function will pop an element off of the front of a list.
 *
 * @param list Pointer to the list to pop an element from
 * @return none
 */
void linkedList_PopFront(linkedList_t* list);

/** @brief linkedListNextElement
 *
 * This function returns a pointer to the next element to the provided element.
 * If the provided element is NULL, it will return the head item on the list.
 *
 * @param list Pointer to the list to get the next element from
 * @param elementPosition Pointer to the list element to get the next item from
 * @return A pointer of the element 
 */
linkedList_Element_t* linkedList_NextElement(linkedList_t* list,
	                                       linkedList_Element_t* elementPosition);


#endif /*LINKEDLIST_H_*/
