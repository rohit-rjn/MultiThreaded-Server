#include<stdio.h>
#include<stdlib.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<pthread.h>
int ID;
union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
};
int state=1;
int sem_init(int no_of_sems, int common_intial_value)
{
    key_t h=ftok(".", state++);

    int sem_id=semget(h, no_of_sems, IPC_CREAT|0666);
    if(sem_id==-1)
    {
        printf("Semaphore init error\n");
        exit(1);
    }
    union semun tmp;
    tmp.val=common_intial_value;
    int i;
    for(i=0; i<no_of_sems; i++)
        semctl(sem_id, i, SETVAL, tmp);
    return sem_id;
}

//Shibin George: below function initializes the semaphore set with the given id, with a common value.
int sem_id_init(int sem_id, int no_of_sems, int common_intial_value)
{
    union semun tmp;
    tmp.val=common_intial_value;
    int i;
    for(i=0; i<no_of_sems; i++)
        semctl(sem_id, i, SETVAL, tmp);
    return sem_id;
}

int sem_init_diff_val(int no_of_sems, int *array)
{
    key_t h=ftok(".", state++);
    int sem_id=semget(h, no_of_sems, IPC_CREAT|0666);
    if(sem_id==-1)
    {
        printf("Semaphore init error\n");
        exit(1);
    }
    union semun tmp;//.array=array;
    //tmp.array=(unsigned short int*)array;
    int i;
    for(i=0; i<no_of_sems; i++)
    {
        tmp.val = array[i];
        semctl(sem_id, i, SETVAL, tmp);
    }

    return sem_id;
}

//Shibin George:below function inits the value of a given semaphore of a given sem_id.
void sem_index_init(int sem_id, int sem_index, int value)
{
    union semun tmp;
    tmp.val = value;
    semctl(sem_id, sem_index, SETVAL, tmp);
}

//Shibin George:below function retruns the value of semaphore at a particular index.
int sem_val(int sem_id, int index)
{
    return semctl(sem_id, index, GETVAL, 0);
}

void sem_change(int sem_id, int sem_no, int amount)
{
    struct sembuf tmp;
    tmp.sem_num=sem_no;
    tmp.sem_flg=0;
    tmp.sem_op=amount;
    if(semop(sem_id, &tmp, 1)==-1)
    {
        printf("Sem_op error\n");
        exit(1);
    }
}

int sem_try_change(int sem_id, int sem_no, int amount)
{
    struct sembuf tmp;
    tmp.sem_num=sem_no;
    tmp.sem_flg=IPC_NOWAIT;
    tmp.sem_op=amount;
    return semop(sem_id, &tmp, 1);

}
