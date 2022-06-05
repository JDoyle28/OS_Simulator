// header files
#include "Simulator.h"

/*
 * Function Name: runSim
 * Algorithm: master driver for simulator operations;
 *            conducts OS simulation with varying scheduling strageties
 *            and varying numbers of processes
 * Precondition: given head pointer to config data and meta data
 * Postcondition: simulation is provided, file output is provided as configured
 * Exceptions: none
 * Notes: none
*/
void runSim(ConfigDataType *configPtr, OpCodeType *metaDataMstrPtr)
   {
    // initialize function/variables
    char timeStr[STD_STR_LEN];
    char statusMessage[STD_STR_LEN];
    FileNode *fileHeadPtr = NULL;
    PCB *PCB_HeadPtr = NULL;
    PCB *PCB_Iterator = NULL;
    int logToCode = configPtr->logToCode;
    int currPid;
    OpCodeType *currOp;
    char ioType[STD_STR_LEN];
    pthread_t opThread;
    int threadTime;


    MMU *memHead = NULL;
    MMU *memIterator = memHead;
    MMU *previous;
    Boolean result;

    // display run banner
    printf("Simulator Run");
    printf("\n-------------\n\n");

    // start timer and display start message
    // display is done manually for first line so that fileHeadPtr is not null
    // when passed
    accessTimer(ZERO_TIMER, timeStr);
    sprintf(statusMessage, " %s, %s", timeStr, "OS: Simulator Start\n");
    if (logToCode == LOGTO_MONITOR_CODE || logToCode == LOGTO_BOTH_CODE)
       {
        printf(statusMessage);
       }
    if (logToCode == LOGTO_FILE_CODE || logToCode == LOGTO_BOTH_CODE)
       {
        fileHeadPtr = addFileNode(fileHeadPtr, statusMessage);
       }

    // if logging solely to file, notify user
    if (configPtr->logToCode == LOGTO_FILE_CODE)
       {
        printf("Logging output to file: %s\n", configPtr->logToFileName);
       }

    // initialize PCBs
    PCB_HeadPtr = initializePCBs(PCB_HeadPtr, metaDataMstrPtr->nextNode,
                                                                configPtr, 0);

    // set and display PCB states
    PCB_Iterator = PCB_HeadPtr;
    while (PCB_Iterator != NULL)
       {
        // set current PCB's state to ready
        PCB_Iterator->currState = READY;

        // display updated state
        accessTimer(LAP_TIMER, timeStr);
        sprintf(statusMessage,
                     " %s, OS: Process %d set to READY state from NEW state,\n",
                                                   timeStr, PCB_Iterator->pid);
        updateDisplay(statusMessage, logToCode, fileHeadPtr);

        // iterate to next pointer
        PCB_Iterator = PCB_Iterator->nextNode;
       }

    // reset PCB iterator
    PCB_Iterator = PCB_HeadPtr;

    // loop until simulator complete
    while ( simComplete( PCB_HeadPtr ) == False )
       {
        // get next PCB
        PCB_Iterator = getNextProcess(PCB_Iterator, configPtr->cpuSchedCode,
                                                logToCode, fileHeadPtr, PCB_HeadPtr);

        //PCB_Iterator = PCB_HeadPtr;
        currPid = PCB_Iterator->pid;

        // set next op code
        PCB_Iterator->opCodePtr = PCB_Iterator->opCodePtr->nextNode;
        currOp = PCB_Iterator->opCodePtr;


        // check for I/O op
        if (compareString(currOp->inOutArg, "in") == STR_EQ ||
            compareString(currOp->inOutArg, "out") == STR_EQ)
           {
            // determine type of I/O for display
            accessTimer(LAP_TIMER, timeStr);
            if (compareString(currOp->inOutArg, "in") == STR_EQ)
               {
                copyString(ioType, "input");
               }
            else
               {
                copyString(ioType, "output");
               }

            // display start of I/O op
            sprintf(statusMessage,
                            " %s, Process: %d, %s %s operation start\n",
                            timeStr, currPid, currOp->strArg1, ioType);
            updateDisplay(statusMessage, logToCode, fileHeadPtr);

            // perform I/O op using thread
            threadTime = currOp->intArg2 * configPtr->ioCycleRate;
            pthread_create(&opThread, NULL, &threadOp, &threadTime);
            pthread_join(opThread, NULL);

            // display I/O op end
            accessTimer(LAP_TIMER, timeStr);
            sprintf(statusMessage,
                            " %s, Process: %d, %s %s operation end\n",
                            timeStr, currPid, currOp->strArg1, ioType);
            updateDisplay(statusMessage, logToCode, fileHeadPtr);
           }

        // otherwise, check for cpu op
        else if (compareString(currOp->strArg1, "process") == STR_EQ)
           {
            // display start of cpu op
            accessTimer(LAP_TIMER, timeStr);
            sprintf(statusMessage,
                        " %s, Process: %d, cpu process operation start\n",
                                                         timeStr, currPid);
            updateDisplay(statusMessage, logToCode, fileHeadPtr);

            // perform cpu op
            threadTime = currOp->intArg2 * configPtr->procCycleRate;
            pthread_create(&opThread, NULL, &threadOp, &threadTime);
            pthread_join(opThread, NULL);

            // display end of cpu op
            accessTimer(LAP_TIMER, timeStr);
            sprintf(statusMessage,
                         " %s, Process: %d, cpu process operation end\n",
                                                        timeStr, currPid);
            updateDisplay(statusMessage, logToCode, fileHeadPtr);
           }

        // otherwise, check for memory access or allocation
        else if ( compareString(currOp->command, "mem") == STR_EQ )
            {

            // check for allocating memory command
            if ( compareString(currOp->strArg1, "allocate") == STR_EQ )
              {
                accessTimer( LAP_TIMER, timeStr );
                sprintf(statusMessage,
                      " %s, Process: %d, attempting mem allocate request\n",
                                                timeStr, currPid);
                updateDisplay(statusMessage, logToCode, fileHeadPtr);


              if ( memHead == NULL )
                  {
                   // returns a new mem node with a boolean value attached
                   memHead = addMem( memHead, currOp->intArg2, currOp->intArg3, configPtr );
                   memIterator = memHead;
                  }
              else
                {
                  // returns a new mem node with a boolean calue attached
                  previous = memIterator;
                  memIterator = addMem( memHead, currOp->intArg2, currOp->intArg3, configPtr);
                  previous->nextNode = memIterator;
                }

              // check if allocation is possible
              if ( memIterator->mmuResult == True )
                {
                  accessTimer( LAP_TIMER, timeStr );
                  sprintf(statusMessage,
                        " %s, Process: %d, successful mem allocate request\n",
                                                  timeStr, currPid);
                  updateDisplay(statusMessage, logToCode, fileHeadPtr);
                }
              // failed allocation attempt
              else
                {
                  accessTimer( LAP_TIMER, timeStr );
                  sprintf(statusMessage,
                        " %s, Process: %d, failed mem allocate request\n",
                                                  timeStr, currPid);
                  updateDisplay(statusMessage, logToCode, fileHeadPtr);
                  PCB_Iterator->currState = EXIT;

                  accessTimer( LAP_TIMER, timeStr );
                  sprintf(statusMessage,
                        " %s, Process: %d, experiences segmentation fault\n",
                                                  timeStr, currPid);
                  updateDisplay(statusMessage, logToCode, fileHeadPtr);
                }
              }

            // if not allocate, then it must be access
            else
              {
                accessTimer( LAP_TIMER, timeStr );
                sprintf(statusMessage,
                      " %s, Process: %d, attempting mem access request\n",
                                                timeStr, currPid);
                updateDisplay(statusMessage, logToCode, fileHeadPtr);

                // retrieves a boolean value if it could fit in one of the data blocks
                result = memAccess( memHead, currOp->intArg2, currOp->intArg3 );

              // if result is false
              if ( result == False )
                {
                  accessTimer( LAP_TIMER, timeStr );
                  sprintf(statusMessage,
                        " %s, Process: %d, failed mem access request\n",
                                                  timeStr, currPid);
                  updateDisplay(statusMessage, logToCode, fileHeadPtr);
                  PCB_Iterator->currState = EXIT;

                  accessTimer( LAP_TIMER, timeStr );
                  sprintf(statusMessage,
                        " %s, Process: %d, experiences segmentation fault\n",
                                                  timeStr, currPid);
                  updateDisplay(statusMessage, logToCode, fileHeadPtr);
                }
              else
                {
                  accessTimer( LAP_TIMER, timeStr );
                  sprintf(statusMessage,
                        " %s, Process: %d, successful mem access request\n",
                                                    timeStr, currPid);
                  updateDisplay(statusMessage, logToCode, fileHeadPtr);
                }
              }
            }

        // check for end of process
        if (compareString(currOp->strArg1, "end") == STR_EQ || PCB_Iterator->currState == EXIT)
           {
            // display process end
            accessTimer(LAP_TIMER, timeStr);
            sprintf(statusMessage, "\n %s, OS: Process %d ended\n",
                                                  timeStr, PCB_Iterator->pid);
            updateDisplay(statusMessage, logToCode, fileHeadPtr);

            // set PCB state to exit
            PCB_Iterator->currState = EXIT;

            // display state change
            accessTimer(LAP_TIMER, timeStr);
            sprintf(statusMessage,
                     " %s, OS: Process %d set to EXIT\n",
                                     timeStr, PCB_Iterator->pid);
            updateDisplay(statusMessage, logToCode, fileHeadPtr);
           }

        // otherwise, assume not at end of process
        else
           {
            // set PCB state back to ready
            PCB_Iterator->currState = READY;
           }
       }

    // display end of opeations
    accessTimer(LAP_TIMER, timeStr);
    sprintf(statusMessage, " %s, OS: System stop\n", timeStr);
    updateDisplay(statusMessage, logToCode, fileHeadPtr);

    // dislay end of sim
    accessTimer(LAP_TIMER, timeStr);
    sprintf(statusMessage, " %s, OS: Simulation end\n", timeStr);
    updateDisplay(statusMessage, logToCode, fileHeadPtr);

    // print output to file, if configured to do so
    if (logToCode == LOGTO_FILE_CODE || logToCode == LOGTO_BOTH_CODE)
       {
        outputToFile(fileHeadPtr, configPtr->logToFileName);
       }

    // free allocated memory
    fileHeadPtr = clearFileData(fileHeadPtr);
    PCB_HeadPtr = clearPCB_Data(PCB_HeadPtr);
   }

/*
 * Function Name: addFileNode
 * Algorithm: recursively adds an output string to the file linked list for
 *            future printing
 * Precondition: given pointer to node in file linked list and c-style string
 *               to be added in new node
 * Postcondition: new node added to linked list with given string to print,
 *                pointer to head node returned
 * Exceptions: none
 * Note: none
 */
FileNode *addFileNode(FileNode *localPtr, char* outputStr)
   {
    // check for null node, insert new node there
    if (localPtr == NULL)
       {
        localPtr = (FileNode*)malloc(sizeof(FileNode));
        copyString(localPtr->printStr, outputStr);
        localPtr->nextNode = NULL;
        return localPtr;
       }

    // recurse to next node if localPtr not null
    localPtr->nextNode = addFileNode(localPtr->nextNode, outputStr);

    // return local pointer
    return localPtr;
   }

/*
 * Function Name: clearFileData
 * Algorithm: recursively iterates through file linked list, returns memory
 *            to OS from the bottom of the list upward
 * Precondition: given linked list, with or without data
 * Postcondition: all node memory, if any, is returned to OS, return pointer
 *                (head) is set to null
 * Exceptions: none
 * Notes: none
 */
FileNode *clearFileData(FileNode *localPtr)
   {
    // check for localPtr not null
    if (localPtr != NULL)
       {
        // recurse to next pointer
        clearFileData(localPtr->nextNode);

        // release memory to OS and set current node to null
        free(localPtr);
        localPtr = NULL;
       }
    // return null to calling function
    return NULL;
   }

/*
 * Function Name: clearPCBData
 * Algorithm: recursively iterates through PCB linked list, returns memory
 *            to OS from the bottom of the list upward
 * Precondition: given linked list, with or without data
 * Postcondition: all node memory, if any, is returned to OS, return pointer
 *                (head) is set to null
 * Exceptions: none
 * Notes: none
 */
PCB *clearPCB_Data(PCB *localPtr)
   {
    // check for localPtr not null
    if (localPtr != NULL)
       {
        // recurse to next pointer
        clearPCB_Data(localPtr->nextNode);

        // release memory to OS and set current node to null
        free(localPtr);
        localPtr = NULL;
       }

    // return null to calling function
    return NULL;
   }


/* Function Name; getShortestProcess
 * Algorithm; returns a list of pcbs in a proper order of shortest to longest
 * Precondtion; a pointer to a pcb
 * Postcondition; a pointer of the head of a sorted list

 * 4-0-1-3-2 is the order it should go
 */
PCB *getShortestProcess ( PCB *localPtr, PCB *head )
    {
    PCB *iterator = NULL;
    PCB *shortestCurJob;
    iterator = head;

    if ( localPtr->currState == EXIT )
      {
        while ( iterator != NULL )
          {
            if( iterator->currState != EXIT )
              {
               localPtr = iterator;
              }
            iterator = iterator->nextNode;
          }
      }

    shortestCurJob = localPtr;
    iterator = head;

    while ( iterator != NULL )
      {
        if ( iterator->timeRemaining < shortestCurJob->timeRemaining &&
                                                  iterator->currState != EXIT )
          {
            shortestCurJob = iterator;
            localPtr = iterator;
          }

        iterator = iterator->nextNode;
      }

    return shortestCurJob;
    }



/*
 * Function Name: getNextProcess
 * Algorithm: finds and returns the PCB of the next process to run based on
 *            the configured scheduling algorithm
 * Precondition: given pointer to PCB of last process run, scheduling code,
 *               and output information
 * Postcondition: pointer to next process returned
 * Exceptions: none
 * Notes: none
 */
PCB *getNextProcess(PCB *localPtr, int schedCode, int logToCode,
                                        FileNode *fileHeadPtr, PCB *head)
   {
    // initialize variables
    char timeStr[STD_STR_LEN];
    char statusMessage[STD_STR_LEN];
    Boolean newProcess = False;

    // check for FCFS-N
    if ( schedCode == CPU_SCHED_FCFS_N_CODE )
       {
        // check for local pointer in exit state
        if (localPtr->currState == EXIT)
           {
            // set local pointer to next PCB
            localPtr = localPtr->nextNode;

            // set new process flag
            newProcess = True;
           }


        // check for new process selected or start of existing process
        if (newProcess == True || compareString(localPtr->opCodePtr->strArg1,
                                                                  "start") == STR_EQ)
          {
            // display new process selected
            accessTimer(LAP_TIMER, timeStr);
            sprintf(statusMessage,
                       " %s, OS: Process %d selected with %d ms remaining\n",
                        timeStr, localPtr->pid, (int)localPtr->timeRemaining);
            updateDisplay(statusMessage, logToCode, fileHeadPtr);

            // display process state change
            accessTimer(LAP_TIMER, timeStr);
            sprintf(statusMessage,
                         " %s, OS: Process %d set from READY to RUNNING\n\n",
                                                          timeStr, localPtr->pid);
            updateDisplay(statusMessage, logToCode, fileHeadPtr);
           }

         // set selected process to running
         localPtr->currState = RUNNING;

         // return local pointer
         return localPtr;

        }

    // else, check SJF-N and
    else if ( schedCode == CPU_SCHED_SJF_N_CODE )
        {

        // give us a list of a now sorted list of PCBs
        PCB *nextjobs = NULL;
        nextjobs = getShortestProcess( localPtr, head );


        // check for new process selected or start of existing process
        if (compareString(nextjobs->opCodePtr->strArg1, "start") == STR_EQ)
          {
            // display new process selected
            accessTimer(LAP_TIMER, timeStr);
            sprintf(statusMessage,
                       " %s, OS: Process %d selected with %d ms remaining\n",
                        timeStr, nextjobs->pid, (int)nextjobs->timeRemaining);
            updateDisplay(statusMessage, logToCode, fileHeadPtr);

            // display process state change
            accessTimer(LAP_TIMER, timeStr);
            sprintf(statusMessage,
                         " %s, OS: Process %d set from READY to RUNNING\n\n",
                                                              timeStr, nextjobs->pid);
            updateDisplay(statusMessage, logToCode, fileHeadPtr);
           }

         // set selected process to running
        nextjobs->currState = RUNNING;

        // return local pointer
        return nextjobs;

        }

   return localPtr;
   }

/*
 * Function Name: initializePCBs
 * Algorithm: recursively builds a linked list of PCBs using given data,
 *            including a calculation of time remaining
 * Precondition: given a pointer to a PCB node to initialize, a pointer to the
 *               start of its process in metadata, a pointer to config data,
 *               and its pid
 * Postcondition: PCB data structure created with appropriate data and a node
 *                for each process
 * Exceptions: none
 * Note: none
 */
PCB *initializePCBs(PCB *localPtr, OpCodeType *localMdPtr,
                                        ConfigDataType *configPtr, int currPid)
   {
    //initialize variables
    int procTotal = 0, ioTotal = 0, totalTime = 0;

    // temp iter
    OpCodeType *iterator = localMdPtr;

    // check for not at end of op codes
    if (compareString(localMdPtr->command, "sys") != STR_EQ &&
        compareString(localMdPtr->strArg1, "end") != STR_EQ)
       {
        // initialize PCB values
        localPtr = (PCB*)malloc(sizeof(PCB));
        localPtr->pid = currPid;
        localPtr->currState = NEW;
        localPtr->timeRemaining = 0;
        localPtr->opCodePtr = localMdPtr;
        localPtr->nextNode = NULL;



        // loop while not at end of op
        while (compareString(iterator->strArg1, "end") != STR_EQ)
           {
            // check for io op
            if (compareString(iterator->inOutArg, "in") == STR_EQ ||
                compareString(iterator->inOutArg, "out") == STR_EQ)
               {
                // add cycles to total io cycles
                ioTotal += iterator->intArg2;
               }

            // otherwise, check for cpu op
            else if (compareString(iterator->strArg1, "process") == STR_EQ)
               {
                // add cycles to total cpu cycles
                procTotal += iterator->intArg2;
               }

            // iterate to next node
            iterator = iterator->nextNode;
           }

        // calculate remaining time
        totalTime += ioTotal * configPtr->ioCycleRate;
        totalTime += procTotal * configPtr->procCycleRate;
        localPtr->timeRemaining = totalTime;

        // increment pid and recursively add next PCB
        currPid++;
        localPtr->nextNode = initializePCBs(localPtr->nextNode,
                                     iterator->nextNode, configPtr, currPid);
       }

    // return local pointer
    return localPtr;
   }

/*
 * Function Name: outputToFile
 * Algorithm: iterates through file linked list, printing each element to the
 *            file as it goes
 * Precondition: given pointer to file linked list and name of file to output
 *               to
 * Postcondition: simulator output is dumped to file
 * Exceptions: none
 * Note: none
 */
void outputToFile(FileNode *iterator, char *fileName)
   {
    // open file for writing
    FILE *filePtr = fopen(fileName, "w");

    // loop across linked list, printing each string to file
    while (iterator != NULL)
       {
        fputs(iterator->printStr, filePtr);
        iterator = iterator->nextNode;
       }

    // close file
    fclose(filePtr);
   }

/*
 * Function Name: isComplete
 * Algorithm: determines whether the simulator is complete by checking if all
 *            processes have finished execution
 * Precondition: given head pointer to PCB data strucutre
 * Postcondition: boolean result of tests returned
 * Exceptions: none
 * Notes: none
 */
Boolean simComplete(PCB *localPtr)
   {
    // initialize variable
    Boolean complete = True;

    // loop while not at end of PCB nodes
    while (localPtr != NULL)
       {
        // check for current process not in exit state
        if (localPtr->currState != EXIT)
           {
            // unset complete flag
            complete = False;
           }

        // iterate to next PCB
        localPtr = localPtr->nextNode;
       }

    // return boolean result of tests
    return complete;
   }

/*
 * Function Name: threadOp
 * Algorithm: thread operation that simulates an I/O or cpu op by running a
 *            timer for a specified time
 * Precondtion: given running time of op code
 * Postcondition: thread exits after timer is run
 * Exeptions: none
 * Notes: none
 */
void *threadOp(void *time)
   {
    // make integer pointer for time
    int *timeVal = time;

    // run timer for specified time and exit
    runTimer(*timeVal);
    pthread_exit(NULL);
   }

/*
 * Function Name: updateDisplay
 * Algorithm: updates simulator display by outputting status to console,
 *            storing it in a linked list, or both, depending on given
 *            flag
 * Precondition: given status message as a c-style string, display flag, and
 *               head pointer to file linked list
 * Postcondition: simulator output is printed and/or updated as specified
 * Exeptions: none
 * Notes: uses addFileNode to conduct linked list operations
 */
void updateDisplay(char *statusMessage, int logFlag, FileNode *headPtr)
   {
    // output to monitor, if configured
    if (logFlag == LOGTO_MONITOR_CODE || logFlag == LOGTO_BOTH_CODE)
       {
        printf(statusMessage);
       }

    // update file output, if configured
    if (logFlag == LOGTO_FILE_CODE || logFlag == LOGTO_BOTH_CODE)
       {
        headPtr = addFileNode(headPtr, statusMessage);
       }
   }

/*
 * Function Name: addMem
 * Algorithm: create linked list of memory coordinates to use
 * Precondtion: given start and end coordinates
 * Postcondition: gives a pointer back
 */
MMU *addMem( MMU *head, int first, int second,  ConfigDataType *configPtr )
  {
    MMU *newMMU;
    // special case with the header being null
    if ( head == NULL )
      {
        newMMU = (MMU*)malloc(sizeof(MMU));
        newMMU->start = first;
        newMMU->end = first + second;
        newMMU->nextNode = NULL;
        newMMU->mmuResult = True;
        if ( newMMU->end > configPtr->memAvailable )
          {
            newMMU->mmuResult = False;
            head = newMMU;
            return newMMU;
          }
        head = newMMU;
        return head;
        //printf("%d, %d\n", tempMem->start, tempMem->end);
      }

    // initialization
    newMMU = (MMU*)malloc(sizeof(MMU));
    newMMU->start = first;
    newMMU->end = first + second;
    newMMU->nextNode = NULL;
    newMMU->mmuResult = False;
    MMU *tempMem = head;

    // check if enough memory available to allocate
    if ( newMMU->end > configPtr->memAvailable )
      {
        newMMU->mmuResult = False;
        return newMMU;
      }

    // loops through every node and checks for overlaps
    while ( tempMem != NULL && tempMem->mmuResult == True )
      {
        if ( (newMMU->start > tempMem->start && newMMU->end > tempMem->end
                                            && newMMU->start < tempMem->end) )
          {
            newMMU->mmuResult = False;
            return newMMU;
          }
        else if ( (newMMU->start < tempMem->start && newMMU->end < tempMem->end ) )
          {
            newMMU->mmuResult = False;
            return newMMU;
          }
        else if ( (newMMU->start > tempMem->start && newMMU->end < tempMem->end) )
          {
            newMMU->mmuResult = False;
            return newMMU;
          }
        else if ( (newMMU->start < tempMem->start && newMMU->end > tempMem->end) )
          {
            newMMU->mmuResult = False;
            return newMMU;
          }
        tempMem = tempMem->nextNode;
      }
    // return a true flagged node for success
    tempMem->nextNode = newMMU;
    newMMU->mmuResult = True;
    return newMMU;
  }


/*
 * Function Name: memAccess
 * Algorithm: looks at all the nodes and looks to see if first and send fit inbetween
 *              the current nodes start and end
 * Precondtion: a header, and 2 values to check
 * Postcondition: a boolean value of the result
 */
Boolean memAccess( MMU *memHead, int first, int second )
  {
    int end = first + second;
    while ( memHead != NULL )
      {
        if (first > memHead->start && end < memHead->end )
          {
            return True;
          }
        memHead = memHead->nextNode;
      }
    return False;
  }
