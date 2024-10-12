#include "ThreadSwitch.h"
#include <stdio.h>
#include <Windows.h>

extern int CurrentThreadIndex;

extern GMThread_t GMThreadList[MAXGMTHREAD];
void Thread1(void*) {
    while (1) {
        printf("Thread1");
        GMSleep(500);
    }
}
void Thread2(void*) {
    while (1) {
        printf("Thread2");
        GMSleep(200);
    }
}

void Thread3(void*) {
    while (1) {
        printf("Thread3");
        GMSleep(10);
    }
}

void Thread4(void*) {
    while (1) {
        printf("Thread4");
        GMSleep(1000);
    }
}


int main()
{
    RegisterGMThread("Thread1", Thread1, NULL);
    RegisterGMThread("Thread2", Thread2, NULL);
    RegisterGMThread("Thread3", Thread3, NULL);
    RegisterGMThread("Thread4", Thread4, NULL);

    while (TRUE) {
        Sleep(20);
        Scheduling();
    }

    return 0;
}
