/*
==========================================================================
* 类ThreadPool是本代码的核心类，类中自动维护线程池的创建和任务队列的派送

* 其中的TaskBase类是封装了任务类
* 其中的TaskBase类是封装了任务完成后的回调任务类

*用法：定义一个ThreadPool变量，派生TaskBase类和TaskBase类，重载两个类中的Run()函数，然后调用ThreadPool的QueueTaskItem()函数即可

Author: TTGuoying

Date: 2018/02/09 19:55

==========================================================================
*/
#pragma once
#include <Windows.h>
#include <list>
#include <queue>
#include <memory>

using std::list;
using std::queue;
using std::shared_ptr;

class TaskBase
{
public:
	TaskBase() { wParam = NULL;  lParam = NULL; }
	TaskBase(WPARAM wParam, LPARAM lParam) { this->wParam = wParam; this->lParam = lParam;  }
	virtual ~TaskBase() { }

	WPARAM wParam;              // 参数1
	LPARAM lParam;              // 参数2
	virtual int Run() = 0;		// 任务函数

};

class TaskCallbackBase
{
public:
	TaskCallbackBase() {}
	virtual ~TaskCallbackBase() {}

	virtual void Run(int result) = 0; // 回调函数

};

class ThreadPool
{
public:
	ThreadPool(size_t minNumOfThread = 2, size_t maxNumOfThread = 0);
	~ThreadPool();

	BOOL QueueTaskItem(shared_ptr<TaskBase> task, shared_ptr<TaskCallbackBase> taskCb, BOOL longFun = FALSE);
	size_t getPoolSize() { return threadList.size(); }

private:
	// 线程类(内部类)
	class Thread
	{
	public:
		Thread();
		~Thread();

		BOOL isBusy();											// 是否有任务在执行
		void ExecuteTask(shared_ptr<TaskBase> t, shared_ptr<TaskCallbackBase> tcb);		// 执行任务

	private:
		BOOL	busy;											// 是否有任务在执行
		BOOL    exit;											// 是否退出
		HANDLE  thread;											// 线程句柄
		shared_ptr<TaskBase>	task;							// 要执行的任务
		shared_ptr<TaskCallbackBase> taskCb;					// 回调的任务
		static unsigned int __stdcall ThreadProc(PVOID pM);		// 线程函数
	};

	// IOCP的通知种类
	enum WAIT_OPERATION_TYPE
	{
		GET_TASK,
		EXIT
	};

	// 待执行的任务类
	class WaitTask
	{
	public:
		WaitTask(shared_ptr<TaskBase> task, shared_ptr<TaskCallbackBase> taskCb, BOOL bLong = FALSE) 
		{
			this->task = task;
			this->taskCb = taskCb;
			this->bLong = bLong;
		}
		~WaitTask() { task = NULL; taskCb = NULL; }

		shared_ptr<TaskBase>	task;					// 要执行的任务
		shared_ptr<TaskCallbackBase> taskCb;			// 回调的任务
		BOOL bLong;										// 是否时长任务
	};

	// 从任务列表取任务的任务
	class GetTaskTask : TaskBase
	{
	public:
		GetTaskTask(ThreadPool *threadPool) { wParam = (WPARAM)threadPool; }
		~GetTaskTask() { }

		int Run()
		{
			ThreadPool *threadPool = (ThreadPool *)wParam;
			BOOL bRet = FALSE;
			DWORD dwBytes = 0;
			WAIT_OPERATION_TYPE opType;
			OVERLAPPED *ol;
			while (WAIT_OBJECT_0 != WaitForSingleObject(threadPool->stopEvent, 0))
			{
				BOOL bRet = GetQueuedCompletionStatus(threadPool->completionPort, &dwBytes, (PULONG_PTR)&opType, &ol, INFINITE);
				// 收到退出标志
				if (EXIT == (DWORD)opType)
				{
					break;
				}
				else if (GET_TASK == (DWORD)opType)
				{
					threadPool->GetTaskExcute();
				}
			}

			return 1;
		}

	};

	void GetTaskExcute();

	CRITICAL_SECTION csThreadLock;
	list<shared_ptr<Thread>> threadList;				// 线程列表

	CRITICAL_SECTION csWaitTaskLock;
	queue<shared_ptr<WaitTask>> waitTaskList;			// 任务列表

	HANDLE					stopEvent;					// 通知线程退出的时间
	HANDLE					completionPort;				// 完成端口
	size_t					maxNumOfThread;
	size_t					minNumOfThread;
	shared_ptr<Thread>		dispatchTaskthread;		// 分发任务的线程
};



