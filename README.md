# ThreadPool

A ThreadPool clss base on IOCP on Windows.

本类配套文章：http://www.cnblogs.com/tanguoying/p/8454637.html

* 类ThreadPool是本代码的核心类，类中自动维护线程池的创建和任务队列的派送

* 其中的TaskBase类是封装了任务类
* 其中的TaskBase类是封装了任务完成后的回调任务类

* 用法：定义一个ThreadPool变量，派生TaskBase类和TaskBase类，重载两个类中的Run()函数，然后调用ThreadPool的QueueTaskItem函()数即可

Author: TTGuoying

Date: 2018/02/09 19:55
