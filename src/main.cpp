/*
 * @Descripttion: main.cpp
 * @Author: Qingao Chai
 * @Date: 2020-02-29 10:56:44
 * @LastEditors: Qingao Chai
 * @LastEditTime: 2020-03-03 19:50:14
 */
#include <iostream>
#include <sstream>
#include <time.h>
#include <windows.h>
#include <omp.h>
using namespace std;

#define MAX_MESSAGE_QUEUE_SIZE 10 //消息队列总长度，容量为9

struct message_queue //消息队列
{
    string *msg;                        //消息数组
    int front, back;                    //队列头、队列尾
    omp_lock_t front_mutex, back_mutex; //队列头锁、队列尾锁
};
enum status
{
    SUCCESS,
    FAILURE
}; //发送/接收消息状态

void init(message_queue &pool);                          //初始化消息队列
void destory(message_queue &pool);                       //销毁消息队列
int getQueueSize(message_queue &pool);                   //计算消息队列中消息个数
status sendMessage(message_queue &pool, string msg);     //生产者发送消息
status receiveMessage(message_queue &pool, string &msg); //消费者接收消息

int main()
{
    int sent_msg_num = 0, total_msg_num = 26;           //已发送消息数量，总共待发送消息数量
    string msgs[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
    string received_message = "";
    int num_threads = 4;
    message_queue pool;
    init(pool);
    omp_lock_t lock;
    omp_init_lock(&lock);
    srand((unsigned)time(NULL));
#pragma omp parallel for
    for (int i = 0; i < num_threads; i++)
    {
        /*生产者 线程0与线程1*/
        if (i < num_threads / 2)
        {
            //发送消息直到所有消息发送完毕
            while (sent_msg_num < total_msg_num)
            {
                omp_set_lock(&lock);
                if (sent_msg_num < total_msg_num)
                {
                    if (sendMessage(pool, msgs[sent_msg_num]) == SUCCESS)
                    {
                        sent_msg_num++;
                    }
                }
                omp_unset_lock(&lock);
            }
        }
        /*生产者 线程2与线程3*/
        else
        {
            //接收消息直到生产者已发送完毕且消息队列为空
            while (getQueueSize(pool) > 0 || sent_msg_num < total_msg_num)
            {
                string msg;
                if (receiveMessage(pool, msg) == SUCCESS)
                {
                    received_message += msg;
                }
            }
        }
    }
    destory(pool);
    omp_destroy_lock(&lock);
    cout << "All received message: \"" << received_message << "\".\n";
    return 0;
}
void init(message_queue &pool)
{
    pool.msg = new string[MAX_MESSAGE_QUEUE_SIZE];
    pool.front = 0;
    pool.back = 0;
    omp_init_lock(&pool.front_mutex);
    omp_init_lock(&pool.back_mutex);
}

void destory(message_queue &pool)
{
    delete[] pool.msg;
    omp_destroy_lock(&pool.front_mutex);
    omp_destroy_lock(&pool.back_mutex);
}

int getQueueSize(message_queue &pool)
{
    int size = (pool.back - pool.front) % MAX_MESSAGE_QUEUE_SIZE;
    return size >= 0 ? size : (size + MAX_MESSAGE_QUEUE_SIZE);
}

status sendMessage(message_queue &pool, string msg)
{
    Sleep(rand()%500);              //延时，便于直观显示
    status flag = FAILURE;
    omp_set_lock(&pool.back_mutex);
    if (getQueueSize(pool) < MAX_MESSAGE_QUEUE_SIZE - 1)
    {
        /*当消息队列不为满时，向队尾添加消息*/
        pool.msg[pool.back] = msg;
        pool.back = (pool.back + 1) % MAX_MESSAGE_QUEUE_SIZE;
        flag = SUCCESS;
        ostringstream os;
        os << "Thread " << omp_get_thread_num() << " sent \"" << msg << "\".-->" << endl;
        cout << os.str();
    }
    omp_unset_lock(&pool.back_mutex);
    return flag;
}

status receiveMessage(message_queue &pool, string &msg)
{
    Sleep(rand()%500+500);          //延时，便于直观显示
    status flag = FAILURE;
    omp_set_lock(&pool.front_mutex);
    if (getQueueSize(pool) != 0)
    {
        /*当消息队列不为空时，从队头获取消息*/
        msg = pool.msg[pool.front];
        pool.front = (pool.front + 1) % MAX_MESSAGE_QUEUE_SIZE;
        flag = SUCCESS;
        ostringstream os;
        os << "\t\t\t\t\t-->Thread " << omp_get_thread_num() << " received \"" << msg << "\"." << endl;
        cout << os.str();
    }
    omp_unset_lock(&pool.front_mutex);
    return flag;
}