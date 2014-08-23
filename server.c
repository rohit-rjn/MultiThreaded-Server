#include<stdio.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<signal.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include "mysem.c"

struct message
{
    int send_index;     // reference ID of the sender
    int recv_index;     // reference ID of the receiver
    char msg[10000]; // message contents
};

void sigusr1(int signum)
{

}

int main()
{
    char pathname[1000];
    getwd(pathname);

    //creating the write-semaphore(two)
    key_t keyw = ftok(pathname, 'A');
    int write_sem_id = semget(keyw, 2, IPC_CREAT|0666);
    if(write_sem_id < 0)
    {
        printf("Semaphore init error\n");
        exit(1);
    }
    sem_index_init(write_sem_id, 0, 1);     //count of semaphore 1 is 1..
    sem_index_init(write_sem_id, 1, 0);     //count of semaphore 2 is 0..
    printf("%d %d\n", sem_val(write_sem_id, 0), sem_val(write_sem_id, 1));
    //creating shared memory for storing process IDs of max 50 processes
    //the first memory location stores the current number of active processes
    int *pids;
    key_t pid_shm_key = ftok(pathname, 'C');
    int pid_shm_id = shmget(pid_shm_key, 60*sizeof(int), IPC_CREAT|0666);
    if(pid_shm_id < 0)
    {
        printf("shmget error\n");
        exit(1);
    }
    pids = (int*)shmat(pid_shm_id, NULL, 0);
    *(pids) = 0;//currently number of processes is 0

    //creating shared memory for storing message contents
    //can store maximum of 10000 characters.
    //Each process/client(max 50) has its own memory-block
    //0th block is server's. Rest are allocated according to reference IDs..
    struct message *m;
    key_t msg_shm_key = ftok(pathname, 'D');
    int msg_shm_id = shmget(msg_shm_key, 60*sizeof(struct message), IPC_CREAT|0666);
    if(msg_shm_id < 0)
    {
        printf("shmget error\n");
        exit(1);
    }
    m = (struct message *)shmat(msg_shm_id, NULL, 0);
    if(m == (struct message*)-1)
    {
        printf("shmat error\n");
        exit(1);
    }

    //creating tot_read_count
    int *count;
    key_t count_shm_key = ftok(pathname, 'H');
    int count_shm_id = shmget(count_shm_key, sizeof(int), IPC_CREAT|0666);
    if(count_shm_id < 0)
    {
        printf("shmget error\n");
        exit(1);
    }
    count = (int*)shmat(count_shm_id, NULL, 0);
    if(count == (int*)-1)
    {
        printf("shmat error\n");
        exit(1);
    }
    *(count) = 0;   // read_count

    printf("%d %d %d %d hell \n", write_sem_id, pid_shm_id, msg_shm_id,  count_shm_id);
    printf("%d %d", sem_val(write_sem_id, 0), sem_val(write_sem_id, 1));
    sleep(5);
    while(1)
    {
        sem_change(write_sem_id, 1, -1);
        if(m->recv_index != -1)         // message is meant for a specific client
        {
            *(count) = 1;                         // message meant for only one client
            //copy contents to reader's memory-block
            (m + m->recv_index)->msg[0] = '\0';
            (m + m->recv_index)->send_index = m->send_index;
            strcpy((m + m->recv_index)->msg, m->msg);
            printf("message: %s sent to %d\n", (m + m->recv_index)->msg, m->recv_index);
            kill(*(pids + m->recv_index), SIGUSR1);
//            while(*(count) > 0)
//            {
//                sleep(1);
//            }
        }
        else                                      //message is to be transmitted to all
        {
            int i;
            *(count) = 0;
            for(i=1; i<=*(pids); i++)
            {
                if(i!=m->send_index)        //send to all processes, exclude the sender
                {
                    (m + i)->msg[0] = '\0';
                    (m + i)->send_index = m->send_index;
                    strcpy((m + i)->msg, m->msg);
                    *(count) = *(count) + 1;    // increase the count of readers
                    printf("message: %s sent to %d\n", (m + i)->msg, i);
                    kill(*(pids + i), SIGUSR1);
//                    while(*(count) > 0)
//                    {
//                        sleep(1);
//                    }
                    printf("gott back\n");
                }
            }
        }
        while(*(count) > 0)
        {
            sleep(1);
        }
        sem_change(write_sem_id, 0, 1);
    }

    return 0;
}

