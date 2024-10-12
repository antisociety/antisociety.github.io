#include "ThreadSwitch.h"
#include <stdio.h>
#include <Windows.h>

//定义线程栈的大小
#define GMTHREADSTACKSIZE 0x80000

//当前线程的索引
int CurrentThreadIndex = 0;

//线程的列表
GMThread_t GMThreadList[MAXGMTHREAD] = { NULL, 0 };

//线程状态的标志
enum FLAGS
{
	GMTHREAD_CREATE = 0x1,
	GMTHREAD_READY = 0x2,
	GMTHREAD_SLEEP = 0x4,
	GMTHREAD_EXIT = 0x8,
};

//启动线程的函数
void GMThreadStartup(GMThread_t* GMThreadp)
{
	GMThreadp->func(GMThreadp->lpParameter);
	GMThreadp->Flags = GMTHREAD_EXIT;
	Scheduling();

	return;
}

//空闲线程的函数
void IdleGMThread(void* lpParameter)
{
	printf("IdleGMThread---------------\n");
	Scheduling();
	return;
}

//向栈中压入一个uint值
void PushStack(unsigned int** Stackpp, unsigned int v)
{
	*Stackpp -= 1;
	**Stackpp = v;

	return;
}

//初始化线程的信息
void initGMThread(GMThread_t* GMThreadp, char* name, void (*func)(void* lpParameter), void* lpParameter)
{
	unsigned char* StackPages;
	unsigned int* StackDWordParam;
	GMThreadp->Flags = GMTHREAD_CREATE;
	GMThreadp->name = name;
	GMThreadp->func = func;
	GMThreadp->lpParameter = lpParameter;
	StackPages = (unsigned char*)VirtualAlloc(NULL, GMTHREADSTACKSIZE, MEM_COMMIT, PAGE_READWRITE);
	ZeroMemory(StackPages, GMTHREADSTACKSIZE);
	GMThreadp->initialStack = StackPages + GMTHREADSTACKSIZE;
	StackDWordParam = (unsigned int*)GMThreadp->initialStack;
	//入栈
	PushStack(&StackDWordParam, (unsigned int)GMThreadp);//startup 函数所需要的参数
	PushStack(&StackDWordParam, (unsigned int)0);//你好奇这里为什么放0，简单来说是为了平衡堆栈，其次是因为调用startup是要参数的，pop startup->eip后 esp也就是这里，进函数后会把mov ebp,esp  然后ebp+8 就是函数默认的参数位置，这也就是这里为什么多push一个四字节。
	PushStack(&StackDWordParam, (unsigned int)GMThreadStartup);
	PushStack(&StackDWordParam, (unsigned int)5);
	PushStack(&StackDWordParam, (unsigned int)7);
	PushStack(&StackDWordParam, (unsigned int)6);
	PushStack(&StackDWordParam, (unsigned int)3);
	PushStack(&StackDWordParam, (unsigned int)2);
	PushStack(&StackDWordParam, (unsigned int)1);
	PushStack(&StackDWordParam, (unsigned int)0);
	//当前线程的栈顶
	GMThreadp->KernelStack = StackDWordParam;
	GMThreadp->Flags = GMTHREAD_READY;
	return;
}

//将一个函数注册为单独线程执行
int RegisterGMThread(char* name, void(*func)(void* lpParameter), void* lpParameter)
{
	int i;
	for (i = 1; GMThreadList[i].name; i++) {
		if (0 == _stricmp(GMThreadList[i].name, name)) {
			break;
		}
	}
	initGMThread(&GMThreadList[i], name, func, lpParameter);

	return (i & 0x55AA0000);
}

//切换线程
__declspec(naked) void SwitchContext(GMThread_t* SrcGMThreadp, GMThread_t* DstGMThreadp)
{
	__asm {
		push ebp
		mov ebp, esp
		push edi
		push esi
		push ebx
		push ecx
		push edx
		push eax

		mov esi, SrcGMThreadp
		mov edi, DstGMThreadp
		mov[esi + GMThread_t.KernelStack], esp
		//经典线程切换，另外一个线程复活
		mov esp, [edi + GMThread_t.KernelStack]

		pop eax  //esp在上面已经切换到新的线程栈中，这个栈再pop eax，拿到的就是保存的esp(初始化的esp/运行时esp)
		pop edx
		pop ecx
		pop ebx
		pop esi
		pop edi
		pop ebp
		ret   //把栈顶的值弹到eip中，在这里弹出的就是startup的地址到eip中
	}
}

//这个函数会让出cpu，从队列里重新选择一个线程执行
void Scheduling(void)
{
	int i;
	int TickCount;
	GMThread_t* SrcGMThreadp;
	GMThread_t* DstGMThreadp;
	TickCount = GetTickCount();
	SrcGMThreadp = &GMThreadList[CurrentThreadIndex];
	DstGMThreadp = &GMThreadList[0];

	for (i = 1; GMThreadList[i].name; i++) {
		if (GMThreadList[i].Flags & GMTHREAD_SLEEP) {
			if (TickCount > GMThreadList[i].SleepMillsecondDot) {
				GMThreadList[i].Flags = GMTHREAD_READY;
			}
		}
		if (GMThreadList[i].Flags & GMTHREAD_READY) {
			DstGMThreadp = &GMThreadList[i];
			break;
		}
	}

	CurrentThreadIndex = DstGMThreadp - GMThreadList;
	SwitchContext(SrcGMThreadp, DstGMThreadp);
	return;
}

void GMSleep(int MilliSeconds)
{
	GMThread_t* GMThreadp;
	GMThreadp = &GMThreadList[CurrentThreadIndex];
	if (GMThreadp->Flags != 0) {
		GMThreadp->Flags = GMTHREAD_SLEEP;
		GMThreadp->SleepMillsecondDot = GetTickCount() + MilliSeconds;
	}

	Scheduling();
	return;
}
