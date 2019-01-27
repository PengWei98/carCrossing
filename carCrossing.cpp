#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <map>
#include <string>
#include <queue>
#include <iostream>
#include <stdio.h>
using namespace std;

//To store how many zone are occupied and judge whether deadlock happened. If lockNum is 4,then deadlock happend.
int lockNum = 0;

//4 zones in the intersection
pthread_mutex_t zoneMutex[4];


//Conditional variables to control the cars to go across the first zone.
pthread_cond_t firstCond[4];

//Conditional variables to control the cars to go across the second zone.
pthread_cond_t enterCond[4];

pthread_mutex_t lockMutex[4];

pthread_mutex_t enterMutex[4];

//To record whether there exists cars is waiting to go across the intersection and lock it.
//0:east, 1:north, 2:west, 3:north
bool waiting[4]={false, false, false,false};

//a car
class Car {
    int dir;
    int id;
public:
    Car(int dir, int id){
        this->dir = dir;
        this->id = id;
    }
    int getDir(){
        return dir;
    }
    int getId(){
        return id;
    }
};

//The cars in the queue are those that haven't reach  the road.
//0:east, 1:north, 2:west, 3:north
queue<Car> cars[4];

//A thread to schedule the cars
void * scheduling(void *arg)
{
    Car *car = (Car*)arg;
    int dir = car->getDir();
    char dirction[6]="";
    //To define the diretions
    if(dir==0){
        strcpy(dirction, "East");
    }
    if(dir==1){
        strcpy(dirction, "North");
    }
    if(dir==2){
        strcpy(dirction, "West");
    }
    if(dir==3){
        strcpy(dirction, "South");
    }
    //The car is waiting and will be signaled when it comes to the intersection
    pthread_mutex_lock(&lockMutex[dir]);
    pthread_cond_wait(&firstCond[dir], &lockMutex[dir]);
    pthread_mutex_unlock(&lockMutex[dir]);
    
    //the car will enter the intersection and lock a zone
    waiting[dir]=true;
    pthread_mutex_lock(&zoneMutex[dir]);
    printf("car %d from %s arrives at crossing\n", car->getId(),dirction);
    lockNum++;

    sleep(1);
    
    //To judge whether a deadlock happened
    if(lockNum==4){
        //To deal with the deadlock
        //let the current car go to release the deadlock
        printf("DEADLOCK: car jam detected, signalling %s to go\n", dirction);
        //The car has gone across the first zone and unlocks it.
        pthread_mutex_unlock(&zoneMutex[dir]);
        
        //To judge whether there are cars in the queue on the left
        if(waiting[(dir+3)%4] == false && cars[(dir+3)%4].size()>0){
            //signal the car on the left
            cars[(dir+3)%4].pop();
            pthread_cond_signal(&firstCond[(dir+3)%4]);
        }
        //The car has leaved the second zone.
        printf("car %d from %s leaving crossing\n", car->getId(), dirction);
        waiting[dir] = false;
        lockNum--;
        
        //To judge whether there are cars has entered the first zone on the left
        if(waiting[(dir+3)%4] == true){
            //signal its left car to go across the second zone
            pthread_cond_signal(&enterCond[(dir+3)%4]);
        }
        //To judge whether there exists cars behind itself
        if(waiting[dir] == false && cars[dir].size()>0){
            //signal the car behind itself
            cars[dir].pop();
            pthread_cond_signal(&firstCond[dir]);
        }
        return NULL;
    }
    
    //To see whether there are cars on the right
    if(waiting[(dir+1)%4]){
        //wait the car on the right to go
        pthread_mutex_lock(&enterMutex[dir]);
        pthread_cond_wait(&enterCond[dir], &enterMutex[dir]);
        pthread_mutex_unlock(&enterMutex[dir]);
    }
    //lock the second zone to go
    pthread_mutex_lock(&zoneMutex[(dir+1)%4]);
    
    //unlock the first zone
    pthread_mutex_unlock(&zoneMutex[dir]);
    
    //To judge whether there are cars in the queue on the left
    if(waiting[(dir+3)%4] == false && cars[(dir+3)%4].size()>0){
        //signal the car on the left
        cars[(dir+3)%4].pop();
        pthread_cond_signal(&firstCond[(dir+3)%4]);
    }
    //unlock the second zone
    pthread_mutex_unlock(&zoneMutex[(dir+1)%4]);
    
    //The car has leaved the second zone.
    printf("car %d from %s leaving crossing\n", car->getId(), dirction);
    waiting[dir] = false;
    lockNum--;
    
    //To judge whether there are cars has entered the first zone on the left
    if(waiting[(dir+3)%4] == true){
        //signal the car on the left
        pthread_cond_signal(&enterCond[(dir+3)%4]);
    }
    //The car has leaved the second zone.
    if(waiting[dir] == false && cars[dir].size()>0){
        //signal the car behind itself
        cars[dir].pop();
        pthread_cond_signal(&firstCond[dir]);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    //To initialize all the locks and conditional variables
    pthread_t carScheduling[100];
    pthread_mutex_init(&lockMutex[0], NULL);
    pthread_mutex_init(&lockMutex[1], NULL);
    pthread_mutex_init(&lockMutex[2], NULL);
    pthread_mutex_init(&lockMutex[3], NULL);

    zoneMutex[0] = PTHREAD_MUTEX_INITIALIZER;
    zoneMutex[1] = PTHREAD_MUTEX_INITIALIZER;
    zoneMutex[2] = PTHREAD_MUTEX_INITIALIZER;
    zoneMutex[3] = PTHREAD_MUTEX_INITIALIZER;
    enterMutex[0] = PTHREAD_MUTEX_INITIALIZER;
    enterMutex[1] = PTHREAD_MUTEX_INITIALIZER;
    enterMutex[2] = PTHREAD_MUTEX_INITIALIZER;
    enterMutex[3] = PTHREAD_MUTEX_INITIALIZER;
    
    firstCond[0] = PTHREAD_COND_INITIALIZER;
    firstCond[1] = PTHREAD_COND_INITIALIZER;
    firstCond[2] = PTHREAD_COND_INITIALIZER;
    firstCond[3] = PTHREAD_COND_INITIALIZER;
    
    enterCond[0] = PTHREAD_COND_INITIALIZER;
    enterCond[1] = PTHREAD_COND_INITIALIZER;
    enterCond[2] = PTHREAD_COND_INITIALIZER;
    enterCond[3] = PTHREAD_COND_INITIALIZER;
    
    //To push all the cars in the queues
    for(int i=0; i<strlen(argv[1]); i++){
        Car *car;
        switch (argv[1][i]) {
            case 'e':
                car = new Car(0, i+1);
                cars[0].push(*car);
                break;
            case 'n':
                car = new Car(1, i+1);
                cars[1].push(*car);
                break;
            case 'w':
                car = new Car(2, i+1);
                cars[2].push(*car);
                break;
            case 's':
                car = new Car(3, i+1);
                cars[3].push(*car);
                break;
            default:
                printf("Invalid input!\n");
                return 0;
        }
        //A car is represented by a thread
        pthread_create(&carScheduling[i], NULL, scheduling, (void*)car);
    }
    sleep(1);
    //To signal all the cars
    for(int i =0; i<4; i++){
        if(cars[i].size()>0){
            cars[i].pop();
            pthread_cond_signal(&firstCond[i]);
        }
    }
    //To wait the end of the threads
    for(int i=0; i<strlen(argv[1]); i++){
        pthread_join(carScheduling[i], NULL);
    }
}

