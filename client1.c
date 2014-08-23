#include<stdio.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<signal.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include "mysem.c"

struct message
{
    int send_index;     // reference ID of the sender
    int recv_index;     // reference ID of the receiver
    char msg[10000]; // message contents
};

pthread_t reader, writer;
int write_sem_id, ref_id, *pids, *count;
struct message *m;

void sigusr1(int signum)
{

}

void read_thread()
{
    while(1)
    {

        signal(SIGUSR1, sigusr1);printf("reader\n");
        pause();        //wait for read signal
        printf("Process %d: %s\n", (m+ref_id)->send_index, (m+ref_id)->msg);
        printf("count->%d\n", *(count)-1);
        *(count) = *(count) - 1;
        (m+ref_id)->msg[0] = '\0';
    }
}

void write_thread()
{
    char arg1[10], arg2[10000], ch;
    while(1)
    {
        scanf("%s", arg1);
        scanf("%c", &ch);                                   // consumes the space in between
        fgets(arg2, sizeof(arg2), stdin);
        //printf("%s\n%s", arg1, arg2);
        printf("\n1..2 -> %s %s\n", arg1, arg2);
        sem_change(write_sem_id, 0, -1);
        if(isdigit(arg1[0]))
        {
            if(atoi(arg1) > *(pids))                          // invalid reference ID
            {
                printf("Invalid reference ID\n");
            }
            else if(ref_id == atoi(arg1))//sender and receiver are the same,
            {
                printf("Self-addressing is not viable\n");
            }
            else
            {
                m->send_index = ref_id;
                m->recv_index = atoi(arg1);
                strcpy(m->msg, arg2);
            }
        }
        else if(arg1[0] == ':' && arg1[1] == 'a' && arg1[2] == 'l' && arg1[3] == 'l')   // ":all"
        {
            m->recv_index = -1;
            strcpy(m->msg, arg2);
        }
        else
        {
            printf("Please enter in the correct format\n");
        }
        sem_change(write_sem_id, 1, 1);
    }
}

int main()
{
    printf("hell\n");
    signal(SIGUSR1, sigusr1);
    char pathname[1000];
    getwd(pathname);

    //creating the write-semaphore(two)
    key_t keyw = ftok(pathname, 'A');
    write_sem_id = semget(keyw, 2, IPC_CREAT|0666);
    if(write_sem_id < 0)
    {
        printf("Semaphore init error\n");
        exit(1);
    }
    printf("%d %d", sem_val(write_sem_id, 0), sem_val(write_sem_id, 1));

    //acquire semaphore to register the process..
    sem_change(write_sem_id, 0, -1);
    printf("%d %d", sem_val(write_sem_id, 0), sem_val(write_sem_id, 1));
    //accessing shared memory for storing process IDs of max 50 processes
    //the first memory location stores the current number of active processes
    key_t pid_shm_key = ftok(pathname, 'C');
    int pid_shm_id = shmget(pid_shm_key, 60*sizeof(int), IPC_CREAT|0666);
    if(pid_shm_id < 0)
    {
        printf("shmget error\n");
        exit(1);
    }
    pids = (int*)shmat(pid_shm_id, NULL, 0);
    *(pids) = *(pids) + 1;  //increase the number of active processes..
    ref_id = *(pids);
    *(pids + ref_id) = getpid();
    printf("Registered PID of this process: %d\nThe reference ID for this process is: %d\n", getpid(), ref_id);

    sem_change(write_sem_id, 0, 1);


    //creating shared memory for storing message contents
    //can store maximum of 10000 characters.
    //Each process/client(max 50) has its own memory-block
    //0th block is server's. Rest are allocated according to reference IDs..

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
    printf("%d %d %d %d %d %d\n", write_sem_id, pid_shm_id, msg_shm_id,  count_shm_id);
    printf("%d %d", sem_val(write_sem_id, 0), sem_val(write_sem_id, 1));
    sleep(5);
    pthread_create(&reader, NULL, (void *)&read_thread, NULL);
    pthread_create(&writer, NULL, (void *)&write_thread, NULL);

    pthread_join(reader, NULL);
    pthread_join(writer, NULL);
    return 0;
}
