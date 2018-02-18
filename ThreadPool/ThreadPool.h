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

#define THRESHOLE_OF_WAIT_TASK  20

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
private:
	// 线程类(内部类)
	class Thread
	{
	public:
		Thread(ThreadPool *threadPool);
		~Thread();

		BOOL isBusy();											// 是否有任务在执行
		void ExecuteTask(shared_ptr<TaskBase> t, shared_ptr<TaskCallbackBase> tcb);		// 执行任务

	private:
		ThreadPool *threadPool;									// 所属线程池
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
		WaitTask(shared_ptr<TaskBase> task, shared_ptr<TaskCallbackBase> taskCb, BOOL bLong)
		{
			this->task = task;
			this->taskCb = taskCb;
			this->bLong = bLong;
		}
		~WaitTask() { task = NULL; taskCb = NULL; bLong = FALSE; }

		shared_ptr<TaskBase>	task;					// 要执行的任务
		shared_ptr<TaskCallbackBase> taskCb;			// 回调的任务
		BOOL bLong;										// 是否时长任务
	};

	// 从任务列表取任务的线程函数
	static unsigned int __stdcall GetTaskThreadProc(PVOID pM)
	{
		ThreadPool *threadPool = (ThreadPool *)pM;
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
		return 0;
	}

	//线程临界区锁
	class CriticalSectionLock
	{
	private:
		CRITICAL_SECTION cs;//临界区
	public:
		CriticalSectionLock() { InitializeCriticalSection(&cs); }
		~CriticalSectionLock() { DeleteCriticalSection(&cs); }
		void Lock() { EnterCriticalSection(&cs); }
		void UnLock() { LeaveCriticalSection(&cs); }
	};


public:
	ThreadPool(size_t minNumOfThread = 2, size_t maxNumOfThread = 10);
	~ThreadPool();

	BOOL QueueTaskItem(shared_ptr<TaskBase> task, shared_ptr<TaskCallbackBase> taskCb = NULL, BOOL longFun = FALSE);	   // 任务入队

private:
	size_t getCurNumOfThread() { return getIdleThreadNum() + getBusyThreadNum(); }	// 获取线程池中的当前线程数
	size_t GetMaxNumOfThread() { return maxNumOfThread - numOfLongFun; }			// 获取线程池中的最大线程数
	void SetMaxNumOfThread(size_t size)			// 设置线程池中的最大线程数
	{ 
		if (size < numOfLongFun)
		{
			maxNumOfThread = size + numOfLongFun;
		}
		else
			maxNumOfThread = size; 
	}					
	size_t GetMinNumOfThread() { return minNumOfThread; }							// 获取线程池中的最小线程数
	void SetMinNumOfThread(size_t size) { minNumOfThread = size; }					// 设置线程池中的最小线程数

	size_t getIdleThreadNum() { return idleThreadList.size(); }	// 获取线程池中的线程数
	size_t getBusyThreadNum() { return busyThreadList.size(); }	// 获取线程池中的线程数
	void CreateIdleThread(size_t size);							// 创建空闲线程
	void DeleteIdleThread(size_t size);							// 删除空闲线程
	Thread *GetIdleThread();									// 获取空闲线程
	void MoveBusyThreadToIdleList(Thread *busyThread);			// 忙碌线程加入空闲列表
	void MoveThreadToBusyList(Thread *thread);					// 线程加入忙碌列表
	void GetTaskExcute();										// 从任务队列中取任务执行

	CriticalSectionLock idleThreadLock;							// 空闲线程列表锁
	list<Thread *> idleThreadList;								// 空闲线程列表
	CriticalSectionLock busyThreadLock;							// 忙碌线程列表锁
	list<Thread *> busyThreadList;								// 忙碌线程列表

	CriticalSectionLock waitTaskLock;
	list<shared_ptr<WaitTask>> waitTaskList;					// 任务列表

	HANDLE					dispatchThrad;						// 分发任务线程
	HANDLE					stopEvent;							// 通知线程退出的时间
	HANDLE					completionPort;						// 完成端口
	size_t					maxNumOfThread;						// 线程池中最大的线程数
	size_t					minNumOfThread;						// 线程池中最小的线程数
	size_t					numOfLongFun;						// 线程池中最小的线程数
};



