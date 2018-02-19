# ThreadPool

A ThreadPool clss base on IOCP on Windows.

本类配套文章：http://www.cnblogs.com/tanguoying/p/8454637.html

* 类ThreadPool是本代码的核心类，类中自动维护线程池的创建和任务队列的派送

* 其中的TaskFun是任务函数

* 其中的TaskCallbackFun是回调函数

*用法：定义一个ThreadPool变量，TaskFun函数和TaskCallbackFun回调函数，然后调用ThreadPool的QueueTaskItem()函数即可

Author: TTGuoying

Date: 2018/02/19 23:15
