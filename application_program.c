#include "system_sam3x.h" 
#include "at91sam3x8.h" 

#include "kernel_functions_february_2019.h"

//void      run(void);
void      Task1(void);
void      Task2(void);
//void      Idle_Task(void);
//void      TimerInt(void);
//void      set_deadline(unsigned int);
void      secondPeriodicScheduler(void);
void      thirdPeriodicScheduler(void);

TCB            *ptr_tcbForTaskOne, *ptr_tcbForTaskTwo/*, *RunningTask*/;
unsigned int   switchesIntoTaskOne, switchesIntoTaskTwo;


/*void set_deadline(unsigned int deadl)
{
  volatile int first=1;
  isr_off();
  first = 1;
  SaveContext();
  if ( first == 1)
  {
    first = 0;
    RunningTask->Deadline = deadl;
    LoadContext();
  }
}
*/
void Task1(void)
{
    unsigned int j;
    j = 0;
    while(1)
    {
        j++;
        set_deadline(5000);
        j--;
        secondPeriodicScheduler();
        j++;
        thirdPeriodicScheduler();
        j--;
    }
}

void Task2(void)
{
    unsigned int k;
    k = 0;
    while(1)
    {
        k++;
        set_deadline(15000);
        k--;
        secondPeriodicScheduler();
        k++;
        thirdPeriodicScheduler();
        k--;
    }
}

//void run(void)
//{
//  RunningTask = ptr_tcbForTaskOne;
//  LoadContext_FirstTime();
//}

//void TimerInt(void)
//{
//  static volatile unsigned int ticks, first=1;
//  if (first == 1) 
//  {
//    first = 0; 
//    ticks = 1;
//  }
//  else 
//  {
//    ticks = ticks + 1;
//  }
//  if ( ticks % 2 == 0 )
//  {  
//    //  if ticks is an even number 
//    RunningTask = ptr_tcbForTaskOne; 
//    switchesIntoTaskOne++;
//  }
//  else
//  {  
//    // if ticks is an odd number 
//    RunningTask = ptr_tcbForTaskTwo;
//    switchesIntoTaskTwo++;
//  }
//}

void secondPeriodicScheduler(void)
{
  static volatile unsigned int ticks2=0, first2=1;
  isr_off();
  ticks2 = ticks2 + 1;
  first2 = 1;
  SaveContext();
  if ( first2 == 1)
  {
      first2 = 0;
      if ( ticks2 % 3 == 0 )
      {  
        /* if ticks is a multiple of 3 */
        RunningTask = ptr_tcbForTaskOne;
        switchesIntoTaskOne++;
      }
      else
      {  
        /* if ticks is not a multiple or 3 */
        RunningTask = ptr_tcbForTaskTwo; 
        switchesIntoTaskTwo++;
      }
      LoadContext();
  }
}


void thirdPeriodicScheduler(void)
{
  static volatile unsigned int ticks3=0, first3=1;
  isr_off();
  ticks3 = ticks3 + 1;
  first3 = 1;
  SaveContext();
  if ( first3 == 1)
  {
      first3 = 0;
      if ( ticks3 % 7 == 0 )
      {  
        /* if ticks is a multiple of 7 */
        RunningTask = ptr_tcbForTaskOne;
        switchesIntoTaskOne++;
      }
      else
      {  
        /* if ticks is not a multiple or 7 */
        RunningTask = ptr_tcbForTaskTwo; 
        switchesIntoTaskTwo++;
      }
      LoadContext();
  }
}


void main(void)
{ 
   unsigned volatile int memory_allocation_problems = 0, dummy;

   SystemInit(); 
   SysTick_Config(10000); /*  configure and enable Sys Tick timer */
   SCB->SHP[((uint32_t)(SysTick_IRQn) & 0xF)-4] =  (0xE0);  
              /*  on this chip priority value used four bits.
                  We have set the lowest possible Priority for the Sys Tick Interrupts */
   isr_off();  /* disable Sys Tick interrupts to protect calloc */

                         //  Check what the priority values are, just in case
   dummy = SCB->SHP[7];  // read the priority value of SVC interrupt
   dummy = SCB->SHP[11]; // read the priority value of Sys Tick interrupt
   
   ptr_tcbForTaskOne = (TCB *)calloc(1,sizeof(TCB));
   if ( ptr_tcbForTaskOne == NULL ) 
   { 
     memory_allocation_problems = 1; 
   }
   else 
   { 
     ptr_tcbForTaskOne->PC   =  Task1;      /*  PC  */
     ptr_tcbForTaskOne->SPSR =  0x21000000; /*  PSR */
     //
     ptr_tcbForTaskOne->StackSeg[STACK_SIZE-2] = 0x21000000;               /*  PSR */
     ptr_tcbForTaskOne->StackSeg[STACK_SIZE-3] = (unsigned int) Task1;      /*  PC  */
     /* the values from StackSeg[STACK_SIZE - 9]
                   to   StackSeg[STACK_SIZE - 4]
        are already zero (because calloc has initialized them so)
     */
     ptr_tcbForTaskOne->SP = &( ptr_tcbForTaskOne->StackSeg[STACK_SIZE-9] ) ;
   }
   
   
   ptr_tcbForTaskTwo = (TCB *)calloc(1,sizeof(TCB));
   if ( ptr_tcbForTaskTwo == NULL )    
   { 
     memory_allocation_problems = 1; 
   }
   else 
   { 
     ptr_tcbForTaskTwo->PC   =  Task2;      /*  PC  */
     ptr_tcbForTaskTwo->SPSR =  0x21000000; /*  PSR */
     //
     ptr_tcbForTaskTwo->StackSeg[STACK_SIZE-2] = 0x21000000;                /*  PSR */
     ptr_tcbForTaskTwo->StackSeg[STACK_SIZE-3] = (unsigned int) Task2;      /*  PC  */
     /* the values from StackSeg[STACK_SIZE - 9]
                   to   StackSeg[STACK_SIZE - 4]
        are already zero (because calloc has initialized them so)
     */
     ptr_tcbForTaskTwo->SP = &( ptr_tcbForTaskTwo->StackSeg[STACK_SIZE-9] ) ;
   }
   
   switchesIntoTaskOne = 0;
   switchesIntoTaskTwo = 0;
   
   if (memory_allocation_problems > 0){ while(1) { } }
   isr_on(); /* enable interrupts */
   run();
} 