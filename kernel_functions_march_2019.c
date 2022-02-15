#include "kernel_functions_march_2019.h"
#include <limits.h>
uint Ticks;
bool KernelMode;
TCB *NextTask, *PreviousTask;

list *ReadyList, *WaitingList, *TimerList;

//---------------------------Custom functions declaration-----------------------
TCB        *alloc_TCBobj();
listobj    *alloc_obj(TCB *task);
list       *alloc_list();
msg        *alloc_msg();
mailbox    *alloc_mailbox();

void        insert_obj(list *prList, listobj *newListobj); //Adds list object to specified list in sorted manner
listobj     *extract_obj(listobj *obj); //Remove list object and return it

void        enqueue(mailbox *mBox, msg *pMsg); //Adds message to last place mailbox
msg         *dequeue(mailbox *mBox); //Dequeues first message in mailbox and return it
void        remove_msg(msg *pMsg, int wait); //Removes message from mailbox and frees memory
//------------------------------------END---------------------------------------
void idle_task(){
  while(1){
    
  }
}

//-------------------------------Task functions---------------------------------

/*Initializes the kernel and its data structures and leaves the kernel in start-up mode*/
exception init_kernel(void){
  Ticks = 0;
  int status = OK;
  
  //Allocates the necessary lists
  ReadyList = alloc_list();
  WaitingList = alloc_list();
  TimerList = alloc_list();
  
  if(ReadyList == NULL || WaitingList == NULL || TimerList == NULL){
    status = FAIL;
  }
  KernelMode = INIT;  //Sets the kernel in start up-mode
  create_task(idle_task, UINT_MAX); //Create an idle task of idle_task()
  return status;
}

/*Creates a task*/
exception create_task(void(*task_body)(), uint deadline){
  int status = OK;
  TCB* newTCB = alloc_TCBobj();  //Allocate memory for a TCB
  if(newTCB == NULL){
    return FAIL; 
  }
  //Sets the pointers of the TCB
  newTCB->Deadline = deadline;
  newTCB->PC = task_body;
  newTCB->SPSR = 0x21000000;
  newTCB->StackSeg[STACK_SIZE-2] = 0x21000000;
  newTCB->StackSeg[STACK_SIZE-3] = (unsigned int) task_body;
  newTCB->SP = &(newTCB->StackSeg[STACK_SIZE - 9]);
  listobj *newListobj = alloc_obj(newTCB); //Allocate memory for a listobj to encapsulate the task
  if(newListobj == NULL){
    return FAIL;
  }
  if(KernelMode == INIT){
    insert_obj(ReadyList, newListobj);
    return status;
  }else{
    isr_off();
    PreviousTask = NextTask;
    insert_obj(ReadyList, newListobj);
    NextTask = ReadyList->pHead->pNext->pTask;
    SwitchContext();
  }
  return status;
}

/*Starts the kernel and the system of created tasks*/
void run(){
  Ticks = 0;
  KernelMode = RUNNING;
  NextTask = ReadyList->pHead->pNext->pTask;
  LoadContext_In_Run();
}

/*Terminates the currently running task*/
void terminate(){
  listobj *terminateObj = extract_obj(ReadyList->pHead->pNext);
  NextTask = ReadyList->pHead->pNext->pTask;
  switch_to_stack_of_next_task();
  free(terminateObj->pTask); //Free task
  free(terminateObj); //Free the task list obj
  LoadContext_In_Terminate(); 
}
//-----------------------------------END----------------------------------------

//--------------------------Inter-Process Communication-------------------------

/*Creates a FIFO mailbox*/
mailbox *create_mailbox(uint nof_msg, uint size_of_msg){
  mailbox *pMailbox = alloc_mailbox();
  pMailbox->nDataSize = size_of_msg; //Set size
  pMailbox->nMaxMessages = nof_msg; //Set max # of messages
  pMailbox->nMessages = 0; //Initialize # of messages
  pMailbox->nBlockedMsg = 0; //Initialize # of blocked messages
  return pMailbox;
}

/*Returns # of messages in a mailbox*/
int no_messages(mailbox* mBox){
  return mBox->nMessages;
}

/*Removes an empty mailbox*/
exception remove_mailbox(mailbox* mBox){
  if(mBox->nMessages == 0){
    remove_msg(mBox->pHead, 0); //Free head and it's sub-pointers
    remove_msg(mBox->pTail, 0); //Free tail and it's sub-pointers
    free(mBox); //Now free the mailbox
    return OK;
  }
  return NOT_EMPTY;
}

/*Sends a message and puts the task in the waiting list until the message is received*/
exception send_wait(mailbox* mBox, void* pData){
  isr_off();
  if(mBox->nMessages > 0 && mBox->nBlockedMsg < 0){ //If recieve message is in mBox
    msg *tempReMsg = dequeue(mBox); //Dequeue the receiving message
    mBox->nMessages--;
    mBox->nBlockedMsg++;
    memcpy(tempReMsg->pData, pData, mBox->nDataSize); //Copy the sending data to the receivings messages data field
    PreviousTask = NextTask;
    insert_obj(ReadyList, extract_obj(tempReMsg->pBlock));
    NextTask = ReadyList->pHead->pNext->pTask;
    remove_msg(tempReMsg, 1);
  }else{ //Create new message to insert into mailbox
    msg *newMsg = (msg*)calloc(1, sizeof(msg));
    if(newMsg == NULL){
      return FAIL;
    }
    newMsg->pBlock = ReadyList->pHead->pNext;
    ReadyList->pHead->pNext->pMessage = newMsg;
    newMsg->pData = pData;
    newMsg->Status = SENDER;
    if(mBox->nMessages == mBox->nMaxMessages){ //If mailbox full
      dequeue(mBox); //Dequeue a message
      mBox->nMessages--;
      mBox->nBlockedMsg--;
    }
    enqueue(mBox, newMsg); //Enqueue the new message
    mBox->nMessages++;
    mBox->nBlockedMsg++;
    PreviousTask = NextTask;
    insert_obj(WaitingList, extract_obj(newMsg->pBlock));
    NextTask = ReadyList->pHead->pNext->pTask;
  }
  SwitchContext();
  if(NextTask->Deadline <= Ticks){//If a task was pushed from the waiting list
    isr_off();
    if(ReadyList->pHead->pNext->pMessage->pNext != NULL && ReadyList->pHead->pNext->pMessage->pPrevious != NULL){ //If the messages simply wasn't read
      remove_msg(ReadyList->pHead->pNext->pMessage, 1); //Free the message and it's sub-pointers
      mBox->nMessages--;
      mBox->nBlockedMsg--;
      ReadyList->pHead->pNext->pMessage = NULL;
    }else{//The message was pushed from a mailbox because it got full
      remove_msg(ReadyList->pHead->pNext->pMessage, 1); //Free the message and it's sub-pointers
      ReadyList->pHead->pNext->pMessage = NULL;
    }
    isr_on();
    return DEADLINE_REACHED;
  }else{
    isr_on();
    return OK;
  }
}

/*Sends a receive message and puts the task in the waiting list until the 
message has read a send message*/
exception receive_wait(mailbox *mBox, void* pData){  
  isr_off();
  if(mBox->nMessages > 0 && mBox->nBlockedMsg >= 0){ //Send message is waiting
    msg *tempSeMsg = dequeue(mBox); //Dequeue the sender message
    mBox->nMessages--;
    memcpy(pData, tempSeMsg->pData, mBox->nDataSize);
    if(mBox->nBlockedMsg > 0){ //Message was of wait type
      mBox->nBlockedMsg--;
      PreviousTask = NextTask;
      insert_obj(ReadyList, extract_obj(tempSeMsg->pBlock));
      NextTask = ReadyList->pHead->pNext->pTask;
      remove_msg(tempSeMsg, 1);
    }else{
      remove_msg(tempSeMsg, 0);
    }
  }else{ //Create new message to insert into mailbox
    msg *newMsg = (msg*)calloc(1, sizeof(msg));
    if(newMsg == NULL){
      return FAIL;
    }
    newMsg->pBlock = ReadyList->pHead->pNext;
    ReadyList->pHead->pNext->pMessage = newMsg;
    newMsg->pData = pData;
    newMsg->Status = RECEIVER;
    if(mBox->nMessages == mBox->nMaxMessages){ //If mailbox full
      dequeue(mBox);
      mBox->nMessages--;
      mBox->nBlockedMsg++;
    }
    enqueue(mBox, newMsg);
    mBox->nMessages++;
    mBox->nBlockedMsg--;
    PreviousTask = NextTask;
    insert_obj(WaitingList, extract_obj(newMsg->pBlock));
    NextTask = ReadyList->pHead->pNext->pTask;
  }
  SwitchContext();
  if(NextTask->Deadline <= Ticks){
    isr_off();
    if(ReadyList->pHead->pNext->pMessage->pNext != NULL && ReadyList->pHead->pNext->pMessage->pPrevious != NULL){
      remove_msg(ReadyList->pHead->pNext->pMessage, 1);
      mBox->nMessages--;
      mBox->nBlockedMsg++;
      ReadyList->pHead->pNext->pMessage = NULL;
    }else{ //Message exists but not in a mailbox
      remove_msg(ReadyList->pHead->pNext->pMessage, 1);
      ReadyList->pHead->pNext->pMessage = NULL;
    }
    isr_on();
    return DEADLINE_REACHED;
  }else{
    isr_on();
    return OK;
  }
}

/*Sends a message to the mailbox and continues execution*/
exception  send_no_wait(mailbox* mBox, void* pData){
  isr_off();
  int status = OK;
  if(mBox->nMessages > 0 && mBox->nBlockedMsg < 0){//If mailbox contains receiver message
    msg *tempReMsg = dequeue(mBox);
    mBox->nMessages--;
    mBox->nBlockedMsg++;
    memcpy(tempReMsg->pData, pData, mBox->nDataSize);
    PreviousTask = NextTask;
    insert_obj(ReadyList, extract_obj(tempReMsg->pBlock));
    NextTask = ReadyList->pHead->pNext->pTask;
    remove_msg(tempReMsg, 1);
    SwitchContext();
  }
  else{ //Create a new message
    msg *newMsg = (msg*)calloc(1, sizeof(msg));
    newMsg->pData = calloc(1, mBox->nDataSize);
    if(newMsg == NULL || newMsg->pData == NULL){
      status = FAIL;
    }
    else{
      newMsg->pBlock = ReadyList->pHead->pNext;
      ReadyList->pHead->pNext->pMessage = newMsg;
      memcpy(newMsg->pData, pData, mBox->nDataSize);
      newMsg->Status = SENDER;
      if(mBox->nMessages == mBox->nMaxMessages){
        remove_msg(mBox->pHead->pNext, 0);
        mBox->nMessages--;
      }
      enqueue(mBox, newMsg);
      mBox->nMessages++;
    }
  }
  isr_on();
  return status;
}

/*Checks the mailbox, if there is a sender it reads it, otherwise continue
execution*/
exception  receive_no_wait(mailbox* mBox, void* pData){
  isr_off();
  int status = OK;
  if(mBox->nMessages > 0 && mBox->nBlockedMsg >= 0){ //Send message is waiting
    msg *tempSeMsg = dequeue(mBox); //Take message from mailbox
    mBox->nMessages--;
    memcpy(pData, tempSeMsg->pData, mBox->nDataSize); //Copy data to message
    if(mBox->nBlockedMsg > 0){ //Message was of wait type
      mBox->nBlockedMsg--;
      PreviousTask = NextTask;
      insert_obj(ReadyList, extract_obj(tempSeMsg->pBlock));
      NextTask = ReadyList->pHead->pNext->pTask;
      remove_msg(tempSeMsg, 1);
      SwitchContext();
    }else{
      remove_msg(tempSeMsg, 0);
    }
  }else{ //No send message found or mailbox is empty
    status = FAIL;
  }
  isr_on();
  return status;
}
//-----------------------------------END----------------------------------------

//---------------------------------Timing---------------------------------------

/*Blocks the calling task until the given number of ticks has expired*/
exception wait(uint nTicks){
  int status;
  isr_off();
  
  ReadyList->pHead->pNext->nTCnt = Ticks + nTicks;
  PreviousTask = NextTask;
  insert_obj(TimerList, extract_obj(ReadyList->pHead->pNext));
  NextTask = ReadyList->pHead->pNext->pTask;
  SwitchContext();
  if(NextTask->Deadline >= Ticks){
    status = DEADLINE_REACHED;
  }else{
    status = OK;
  }
  isr_on();
  return status;
}

/*Sets the tick counter to the given value*/
void set_ticks(uint nTicks){
  Ticks = nTicks; 
}

/*Returns the current value of the tick counter*/
uint ticks(void){
  return Ticks; 
}

/*Returns the deadline of the currently running task*/
uint deadline(void){
  return NextTask->Deadline;
}

/*Sets the deadline of the currently running task*/
void set_deadline(uint deadline){  
  isr_off();
  NextTask->Deadline = deadline;
  PreviousTask = NextTask;
  insert_obj(ReadyList, extract_obj(ReadyList->pHead->pNext));
  NextTask = ReadyList->pHead->pNext->pTask;
  SwitchContext();
}

/*Checks if the deadline of a task in either timer list or waiting list has been
reached, moves it to ready list if it has been reached*/
void TimerInt(void)
{
  Ticks++;
  /*Check TimerList*/
  listobj *tempLObj = TimerList->pHead->pNext;
  while(tempLObj != TimerList->pTail){
    if(tempLObj->pTask->Deadline <= Ticks || tempLObj->nTCnt <= Ticks){
      tempLObj = tempLObj->pNext;
      PreviousTask = NextTask;
      insert_obj(ReadyList, extract_obj(tempLObj->pPrevious));
      NextTask = ReadyList->pHead->pNext->pTask;
    }else{
      tempLObj = tempLObj->pNext;
    }
  }
  
  /*Check WaitingList as long as */
  while(WaitingList->pHead->pNext != WaitingList->pTail && WaitingList->pHead->pNext->pTask->Deadline <= Ticks){
    PreviousTask = NextTask;
    insert_obj(ReadyList, extract_obj(WaitingList->pHead->pNext));
    NextTask = ReadyList->pHead->pNext->pTask;
  }
}
//-----------------------------------END----------------------------------------

//---------------------------LinkedList & Mailbox-------------------------------
TCB *alloc_TCBobj(){
   TCB *tempTCB;
   tempTCB = (TCB*)calloc(1, sizeof(TCB));
   if(tempTCB == NULL){
     return NULL;
   }
   
   return tempTCB;
}

listobj *alloc_obj(TCB *task){
  listobj *templistobj;
  templistobj = (listobj*)calloc(1, sizeof(listobj));
  if(templistobj == NULL){
    return NULL;
  }
  templistobj->pTask = task;
  return templistobj;
}

list *alloc_list(){
  list *tempList;
  listobj *head = alloc_obj(NULL);
  listobj *tail = alloc_obj(NULL);
  if(head == NULL || tail == NULL){
    return NULL;
  }
  tempList = (list*)calloc(1, sizeof(list));
  if(tempList == NULL){
    return NULL;
  }else{
    tempList->pHead = head;
    tempList->pTail = tail;
    
    tempList->pHead->pNext = tempList->pTail;
    tempList->pHead->pPrevious = NULL;
    tempList->pTail->pNext = NULL;
    tempList->pTail->pPrevious = tempList->pHead;
    
    return tempList;
  }
}

msg *alloc_msg(listobj *pBlock){
   msg *tempMsg;
   tempMsg = (msg*)calloc(1, sizeof(msg));
   tempMsg->pData = (char*)calloc(1, sizeof(char));
   if(tempMsg == NULL || tempMsg->pData == NULL){
     return NULL;
   }
   tempMsg->pBlock = pBlock;
   pBlock->pMessage = tempMsg;
   return tempMsg;
}

mailbox *alloc_mailbox(){
  mailbox *tempMB;
  msg     *head = (msg*)calloc(1, sizeof(msg));
  msg     *tail = (msg*)calloc(1, sizeof(msg));
  tempMB = (mailbox*)calloc(1, sizeof(mailbox));
  if(tempMB == NULL || head == NULL || tail == NULL){
    return NULL;
  }
  else{
    tempMB->pHead = head;
    tempMB->pTail = tail;
    
    tempMB->pHead->pNext = tempMB->pTail;
    tempMB->pHead->pPrevious = NULL;
    tempMB->pTail->pNext = NULL;
    tempMB->pTail->pPrevious = tempMB->pHead;
    
    return tempMB;
  }
}

/*Inserts an object into a list and sorts it by deadline*/
void insert_obj(list *prList, listobj *newListobj){
  
  /*Check if list is not empty*/
  if(prList->pHead->pNext != prList->pTail){
    
    /*New nodes deadline lesser than first node*/
    if(newListobj->pTask->Deadline < prList->pHead->pNext->pTask->Deadline){
      newListobj->pNext = prList->pHead->pNext;
      newListobj->pPrevious = prList->pHead;
      prList->pHead->pNext->pPrevious = newListobj;
      prList->pHead->pNext = newListobj;
    }
    
    /*New nodes deadline greater than last node*/
    else if(newListobj->pTask->Deadline > prList->pTail->pPrevious->pTask->Deadline){
      newListobj->pNext = prList->pTail;
      newListobj->pPrevious = prList->pTail->pPrevious;
      prList->pTail->pPrevious->pNext = newListobj;
      prList->pTail->pPrevious = newListobj;
    }
    
    /*Iterate through list and insert new node*/
    else{
      listobj *temp = prList->pHead->pNext;
      while(temp->pTask->Deadline < newListobj->pTask->Deadline){
        temp = temp->pNext;
      }
      temp->pPrevious->pNext = newListobj;
      newListobj->pPrevious = temp->pPrevious; 
      temp->pPrevious = newListobj; 
      newListobj->pNext = temp;
    }
  }
    /*prList is empty so new node is inserted between pHead and pTail*/
  else{
    newListobj->pNext = prList->pTail;
    newListobj->pPrevious = prList->pHead;
    prList->pHead->pNext = newListobj;
    prList->pTail->pPrevious = newListobj;
  }
}

/*Extracts an object from a doubly linked list and returns it*/
listobj *extract_obj(listobj *obj){
  obj->pNext->pPrevious = obj->pPrevious;
  obj->pPrevious->pNext = obj->pNext;
  
  obj->pNext = NULL;
  obj->pPrevious = NULL;
  
  return obj;
}

void  enqueue(mailbox *mBox, msg *pMsg){
  pMsg->pPrevious = mBox->pTail->pPrevious;
  pMsg->pNext = mBox->pTail;
  mBox->pTail->pPrevious->pNext = pMsg;
  mBox->pTail->pPrevious = pMsg;
}

msg *dequeue(mailbox *mBox){
  if(mBox->nMessages != 0){
    msg *tempMsg = mBox->pHead->pNext;
    mBox->pHead->pNext->pNext->pPrevious = mBox->pHead;
    mBox->pHead->pNext = mBox->pHead->pNext->pNext;
    tempMsg->pNext = NULL;
    tempMsg->pPrevious = NULL;
    return tempMsg;
  }else{
    return NULL; 
  }
}

void remove_msg(msg *pMsg, int wait){
  if(pMsg->pNext != NULL && pMsg->pPrevious != NULL){
    pMsg->pNext->pPrevious = pMsg->pPrevious;
    pMsg->pPrevious->pNext = pMsg->pNext;
  }
  if(wait == 0){
    free(pMsg->pData);
  }
  free(pMsg);
}

//-----------------------------------END----------------------------------------
