#include "stdafx.h"
#include "ThreadPool.h"
#include <process.h>


ThreadPool::ThreadPool(size_t minNumOfThread, size_t maxNumOfThread) :
	getTaskTask((TaskBase*)new GetTaskTask(this))
{
	this->minNumOfThread = minNumOfThread;
	this->maxNumOfThread = maxNumOfThread;
	InitializeCriticalSection(&csThreadLock);
	InitializeCriticalSection(&csWaitTaskLock);
	stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);

	EnterCriticalSection(&csThreadLock);
	threadList.clear();
	for (size_t i = 0; i < minNumOfThread; i++)
	{
		Thread *thread = new Thread;
		threadList.push_back(thread);
	}
	Thread *thread = new Thread;
	thread->ExecuteTask(getTaskTask, NULL);
	threadList.push_back(thread);
	LeaveCriticalSection(&csThreadLock);
}

ThreadPool::~ThreadPool()
{
	SetEvent(stopEvent);
	PostQueuedCompletionStatus(completionPort, 0, (DWORD)EXIT, NULL);
	EnterCriticalSection(&csThreadLock);
	list<Thread *>::iterator it = threadList.begin();
	for ( ; it != threadList.end(); it++)
	{
		Thread *thread = *it;
		delete thread;
	}
	threadList.clear();
	LeaveCriticalSection(&csThreadLock);

	
	CloseHandle(stopEvent);
	DeleteCriticalSection(&csThreadLock);
	DeleteCriticalSection(&csWaitTaskLock);
}

BOOL ThreadPool::QueueTaskItem(shared_ptr<TaskBase> task, shared_ptr<TaskCallbackBase> taskCb)
{
	EnterCriticalSection(&csWaitTaskLock);
	shared_ptr<WaitTask> waitTask(new WaitTask(task, taskCb));
	waitTaskList.push(waitTask);
	LeaveCriticalSection(&csWaitTaskLock);
	PostQueuedCompletionStatus(completionPort, 0, (DWORD)GET_TASK, NULL);
	return TRUE;
}

void ThreadPool::GetTaskExcute()
{
	Thread *thread = NULL;
	shared_ptr<WaitTask> waitTask = NULL;

	EnterCriticalSection(&csWaitTaskLock);
	if (waitTaskList.size() > 0)
	{
		waitTask = waitTaskList.front();
		waitTaskList.pop();
	}
	LeaveCriticalSection(&csWaitTaskLock);
	if (waitTask == NULL)
	{
		return;
	}

	EnterCriticalSection(&csThreadLock);
	list<Thread *>::iterator it = threadList.begin();
	for (; it != threadList.end(); it++)
	{
		if (!(*it)->isBusy())
		{
			thread = *it;
			break;
		}
	}
	if (it == threadList.end() && getPoolSize() < maxNumOfThread)
	{
		thread = new Thread;
		threadList.push_back(thread);
	}
	if (thread != NULL)
	{
		thread->ExecuteTask(waitTask->task, waitTask->taskCb);
	}
	LeaveCriticalSection(&csThreadLock);

	if (thread == NULL)
	{
		EnterCriticalSection(&csWaitTaskLock);
		waitTaskList.push(waitTask);
		LeaveCriticalSection(&csWaitTaskLock);
		PostQueuedCompletionStatus(completionPort, 0, (DWORD)GET_TASK, NULL);
	}
	
}


ThreadPool::Thread::Thread() :
	busy(FALSE),
	thread(INVALID_HANDLE_VALUE),
	task(NULL),
	taskCb(NULL),
	exit(FALSE)
{
	thread = (HANDLE)_beginthreadex(0, 0, ThreadProc, this, CREATE_SUSPENDED, 0);
}

ThreadPool::Thread::~Thread()
{
	exit = TRUE;
	task = NULL;
	taskCb = NULL;
	ResumeThread(thread);
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);
}

BOOL ThreadPool::Thread::isBusy()
{
	return busy;
}

void ThreadPool::Thread::ExecuteTask(shared_ptr<TaskBase> t, shared_ptr<TaskCallbackBase> tcb)
{
	busy = TRUE;
	task = t;
	taskCb = tcb;
	ResumeThread(thread);
}

unsigned int ThreadPool::Thread::ThreadProc(PVOID pM)
{
	Thread *pThread = (Thread*)pM;

	while (true)
	{
		if (pThread->exit)
			break; //Ïß³ÌÍË³ö

		if (pThread->task == NULL && pThread->taskCb == NULL)
		{
			pThread->busy = FALSE;
			SuspendThread(pThread->thread);
			continue;
		}

		int resulst = pThread->task->Run();
		if(pThread->taskCb)
			pThread->taskCb->Run(resulst);
		pThread->task = NULL;
		pThread->taskCb == NULL;
		pThread->busy = FALSE;
		SuspendThread(pThread->thread);
	}

	return 0;
}
