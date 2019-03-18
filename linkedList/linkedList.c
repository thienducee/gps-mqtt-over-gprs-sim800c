/*******************************************************************************
 * Copyright (c) 2009, 2014 VNPT Technology Company.
 *
 * Contributors:
 *       Filename:  linkedList.c
 *
 *    Description:  the source file
 *
 *        Version:  1.0
 *        Created:  2017-6-18 15:58:20
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  Le Thien Duc, thienduc.ee@gmail.com - initial implementation
 *   Organization:  none
 *******************************************************************************/
 
#include "linkedlist.h"
#include <stdlib.h>

linkedList_t* linkedList_Init(void)
{
  linkedList_t* list = (linkedList_t*)malloc(sizeof(linkedList_t));
  if (list != NULL) {
    memset(list, 0, sizeof(linkedList_t));
  }
  return list;
}

void linkedList_Deinit(linkedList_t* list)
{
  while (list->count > 0) {
    linkedList_PopFront(list);
  }
  free(list);
}

void linkedList_PushBack(linkedList_t* list, void* content)
{ 
  linkedList_Element_t* element = (linkedList_Element_t*)malloc(sizeof(linkedList_Element_t));
  if (element != NULL) {
    element->content = content;
    element->next = NULL;
    if (list->head == NULL) {
      list->head = element;
    } else {
      list->tail->next = element;
    }
    list->tail = element;
    ++(list->count);
  }
}

void linkedList_PopFront(linkedList_t* list)
{
  if (list->count > 0) {
    linkedList_Element_t* head = list->head;
    if (list->tail == head) {
      list->tail = NULL;
    }
    list->head = list->head->next;
    free(head);
    --(list->count);
  }
}

linkedList_Element_t* linkedList_NextElement(linkedList_t* list,
                                           linkedList_Element_t* elementPosition)
{
  if (elementPosition == NULL) {
    return list->head;
  } else {
    return elementPosition->next;
  }
}
