////Testing functions of linked_list.h and kernel_functions
//#include "assert.h"
//#include "stdio.h"
//#include "system_sam3x.h" 
//#include "at91sam3x8.h" 
//#include "kernel_functions_march_2019.h"
//
//extern list* ReadyList, WaitingList, TimerList;
//extern uint Ticks;
//mailbox* YouGotMail;
//
//
//int isEqualPointer(void *comp1, void *comp2);
//int isNotEqualPointer(void *comp1, void *comp2);
//int isEqualInt(int comp1, int comp2);
//int isNotEqualInt(int comp1, int comp2);
//int isListEmpty(list *this_list);
//
//int isEqualPointer(void *comp1, void *comp2){
//  if(comp1 == comp2){
//    return 1;
//  }
//  return 0;
//}
//
//int isNotEqualPointer(void *comp1, void *comp2){
//  if(comp1 != comp2){
//    return 1;
//  }
//  return 0;
//}
//
//int isEqualInt(int comp1, int comp2){
//  if(comp1 == comp2){
//    return 1;
//  }
//  return 0;
//}
//
//int isNotEqualInt(int comp1, int comp2){
//  if(comp1 != comp2){
//    return 1;
//  }
//  return 0;
//}
//
//int isListEmpty(list *this_list){
//  if(isEqualPointer(this_list->pHead->pNext, this_list->pTail) &&
//     isEqualPointer(this_list->pHead->pPrevious, this_list->pHead) &&
//     isEqualPointer(this_list->pTail->pNext, this_list->pTail) &&
//     isEqualPointer(this_list->pTail->pPrevious, this_list->pHead))
//  {
//    return 1;   
//  }
//  return 0;
//}
//
//int isListNotEmpty(list *this_list){
//  if(isListEmpty(this_list) == 0){
//    return 1;
//  }
//  return 0;
//}
//
//int isListSorted(list *this_list){
//  listobj *temp = this_list->pHead->pNext;
//  while(temp != this_list->pTail){
//    if(temp->pTask->Deadline > temp->pNext->pTask->Deadline
//       && temp->pNext != this_list->pTail)
//    {
//      return 0;
//    }
//    temp = temp->pNext;
//  }
//  return 1;
//}
//
//int sizeOf(list *pList){
//  int i = 0;
//  listobj *temp = pList->pHead->pNext;
//  
//  while(temp != pList->pTail){
//    i++; 
//  }
//  
//  return i;
//}
//
////void task1(){
////  int tal1 = 1;
////  send_wait(YouGotMail, &tal1);
////  printf("%i", tal1);
////  terminate();
////}
////
////void task2(){
////  int tal1 = 2;
////  send_wait(YouGotMail, &tal1);
////  printf("%i", tal1);
////  terminate();
////}
////
////void task3(){
////  int tal1 = 3;
////  receive_wait(YouGotMail, &tal1);
////  printf("%i", tal1);
////  terminate();
////}
////
////void task4(){
////  int tal1 = 4;
////  receive_no_wait(YouGotMail, &tal1);
////  printf("%i", tal1);
////  terminate();
////}
//
//void taskSW1(){
//  char b = 'b';
//  send_wait(YouGotMail, &b);
//  terminate();
//}
//void taskSW2(){
//  char c = 'c';
//  send_wait(YouGotMail, &c);
//  terminate();
//}
//
//main(){
////  list *pList = alloc_list();
////  TCB *pTCB = alloc_TCBobj();
////  listobj *pListObj = alloc_obj(pTCB);
////  assert(isEqualPointer(pList->pTail, pList->pHead));
////  assert(isListEmpty(pList));
////  insert_obj(pList, pListObj);
////  assert(isEqualPointer(pList->pHead->pNext, pListObj));
////  assert(isEqualPointer(pList->pTail->pPrevious, pListObj));
////  assert(isListNotEmpty(pList));
////  extract_obj(pListObj);
////  assert(isListEmpty(pList));
//
////  insert_obj(pList, pListObj2);
////  insert_obj(pList, pListObj1);
////  insert_obj(pList, pListObj4);
////  insert_obj(pList, pListObj3);
//  SystemInit();
//  SysTick_Config(100000);
//  SCB->SHP[((uint32_t)(SysTick_IRQn) & 0xF)-4] = (0xE0);
//  isr_off();
//  init_kernel();
//  
////  create_task(task1, 10);
////  create_task(task2, 20);
////  create_task(task3, 30);
////  create_task(task4, 40);
//  
//  create_task(taskSW1, 10);
//  create_task(taskSW2, 20);
//  
//  YouGotMail = create_mailbox(1, sizeof(char));
//  
//  run();
//  
//  //assert(isListSorted(ReadyList));
//  
//  return 0;
//}