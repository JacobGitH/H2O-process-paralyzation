#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <pthread.h>


// arguments
typedef struct params{
    int NO;
    int NH;
    int TI;
    int TB;
} param_t;

//semaphores
sem_t *test,
      *hydro, 
      *oxy,
      *barrier1,
      *barrier2,
      *barrier3,
      *mutex,
      *processWrite,
      *mutex2;

//shared memory
int *processCount;
int *hydroCount = 0;
int *oxyCount = 0;
int *moleculeCount = 0;
int *count = 0;
int *fullMolecule;
int *tmp = 0;


int *numOfAllHydro;
int *numOfAllOxy;

FILE *fp;

//function for setting argv into struct
int setParams(int argc, char *argv[], param_t *params){
    if(argc != 5)
        return -1;
    char *err = NULL;

    params->NO = strtol(argv[1], &err, 10);
    params->NH = strtol(argv[2], &err, 10);
    params->TI = strtol(argv[3], &err, 10);
    params->TB = strtol(argv[4], &err, 10);
    if(*err != 0 || params->TB > 1000 || params->TI > 1000){
        fprintf(stderr, "chyba pri cteni/zadani argumentu");
        exit(1);
    }
    return 0;

    
}

//seting up shared memory
int setShaMem(){
    processCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    hydroCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    oxyCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    moleculeCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    fullMolecule = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    tmp = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    processWrite = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    numOfAllHydro = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    numOfAllOxy = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    test = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    hydro = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0); 
    oxy = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    barrier1 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    barrier2 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    barrier3 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    mutex = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0); 
    mutex2 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0); 
    int testPom = sem_init(test, 1, 1);
    int procesWritePom = sem_init(processWrite, 1, 1);
    int mutexPom = sem_init(mutex, 1, 1);
    int oxyPom = sem_init(oxy, 1, 0);
    int hydroPom = sem_init(hydro,1, 0); 
    int mutex2Pom = sem_init(mutex2, 1, 1);
    int barrier1Pom = sem_init(barrier1, 1, 0);
    int barrier2Pom = sem_init(barrier2, 1, 1);
    if(processCount == MAP_FAILED || hydroCount == MAP_FAILED || oxyCount == MAP_FAILED || moleculeCount == MAP_FAILED || test == MAP_FAILED || fullMolecule == MAP_FAILED ||
        tmp== MAP_FAILED || test == MAP_FAILED || processCount == MAP_FAILED || hydro == MAP_FAILED || oxy == MAP_FAILED || barrier1 == MAP_FAILED ||
        barrier2 == MAP_FAILED || barrier3 == MAP_FAILED || mutex == MAP_FAILED || numOfAllHydro == MAP_FAILED || numOfAllOxy == MAP_FAILED ||
        testPom == -1 || procesWritePom == -1 || mutexPom == -1 || mutex2Pom == -1 || barrier1Pom == -1 || barrier2Pom == -1 || oxyPom == -1 
        || hydroPom == -1){
            fprintf(stderr, "chyba pri vytvareni shared memory");
            exit(1);
        }
        return 0;
}
//function that destroys aloc semaphores
void cleanup(){
    fclose(fp);
    if(sem_destroy(test) == -1 || sem_destroy(processWrite) == -1 || sem_destroy(mutex) == -1 || sem_destroy(oxy) == -1 ||
     sem_destroy(hydro) == -1 || sem_destroy(mutex2) == -1 || sem_destroy(barrier1) == -1 || sem_destroy(barrier2) == -1){
         fprintf(stderr, "chyba pri destroy semaphoru");
         exit(1);

     }
}


//function that generates hzdrogen
void  hydroGen(int i, int sleepTime){
    i++;

    sem_wait(processWrite);
    fprintf(fp, "%d: H %d: started\n", *processCount, i);
    fflush(fp);
    *processCount = *processCount + 1;
    sem_post(processWrite);

    sem_wait(processWrite);
    usleep(sleepTime);
    fprintf(fp, "%d: H %d: going to queue\n", *processCount, i);
    fflush(fp);
    *processCount = *processCount + 1;
    sem_post(processWrite);
    
    sem_wait(mutex);
    *hydroCount = *hydroCount + 1;
    if(*hydroCount >= 2 && *oxyCount >= 1){
        sem_post(hydro);
        sem_post(hydro);
        *hydroCount = *hydroCount -2;
        sem_post(oxy);
        *oxyCount = *oxyCount - 1; 
    }else{
        
        sem_post(mutex);
    }
     
    sem_wait(hydro);
		if(*numOfAllHydro > 1000){
			fprintf(fp, "%d: H %d: not enough O or H\n", *processCount, i);
			fflush(fp);
			*processCount = *processCount + 1;
			return;
		}
		*numOfAllHydro = *numOfAllHydro - 1;
    //bond()

    sem_wait(processWrite);
    
    usleep(sleepTime);
    fprintf(fp, "%d: H %d: creating molecule %d\n", *processCount, i, *fullMolecule/3);
    fflush(fp);
    
    *processCount = *processCount + 1;
    sem_post(processWrite);

    
    sem_wait(mutex2);
    *count = *count + 1;
    if(*count == 3){
        sem_wait(barrier2);
        sem_post(barrier1);
    }
    sem_post(mutex2);
    
    sem_wait(barrier1);
    sem_post(barrier1);

    //critical point

    sem_wait(processWrite);
        fprintf(fp, "%d: H %d: molecule %d created\n", *processCount, i, *fullMolecule/3);
        fflush(fp);
        *fullMolecule = *fullMolecule +1;
        *processCount = *processCount + 1;
    sem_post(processWrite);
		if(*numOfAllHydro < 2 || *numOfAllOxy < 1){
			*numOfAllHydro = 10000;
			for (int i = 0; i < 1000; ++i) {
				sem_post(oxy);
				sem_post(hydro);
			}
		}


    sem_wait(mutex2);
    *count = *count - 1;
    if (*count == 0){
        sem_wait(barrier1);
        sem_post(barrier2);
    }
    sem_post(mutex2);

    sem_wait(barrier2);
    sem_post(barrier2);

    

   

}

//funcion that generates oxygen
void oxyGen(int i, int sleepTime){
    
    i++;
    sem_wait(processWrite);
    fprintf(fp, "%d: O %d: started\n", *processCount, i);
    fflush(fp);
    *processCount = *processCount + 1;
    sem_post(processWrite);

    sem_wait(processWrite);
    usleep(sleepTime);
    fprintf(fp, "%d: O %d: going to queue\n", *processCount, i);
    fflush(fp);
    *processCount = *processCount + 1;
    sem_post(processWrite);
    
    sem_wait(mutex);
    *oxyCount = *oxyCount + 1;
    
    
    
    
    if(*hydroCount >= 2){
        sem_post(hydro);
        sem_post(hydro);
        *hydroCount = *hydroCount -2;
        sem_post(oxy);
        *oxyCount = *oxyCount -1;
    }else{
        sem_post(mutex);
    }

    sem_wait(oxy);
		if(*numOfAllHydro > 1000){
			fprintf(fp, "%d: O %d: not enough H\n", *processCount, i);
			fflush(fp);
			*processCount = *processCount + 1;
			return;
		}

    *numOfAllOxy = *numOfAllOxy - 1;
    sem_post(mutex);  // special mutex

    sem_wait(processWrite);
    usleep(sleepTime);
    fprintf(fp, "%d: O %d: creating molecule %d\n", *processCount, i, *fullMolecule/3);
    fflush(fp);
    
    *processCount = *processCount + 1;
    sem_post(processWrite);
    

    sem_wait(mutex2);
    *count = *count +1;
    if(*count == 3){
        sem_wait(barrier2);
        sem_post(barrier1);
    }

    sem_post(mutex2);

    sem_wait(barrier1);
    sem_post(barrier1);
   
    //critical
    sem_wait(processWrite);
        fprintf(fp, "%d: O %d: molecule %d created\n", *processCount, i, *fullMolecule/3);
        fflush(fp);
        *fullMolecule = *fullMolecule +1;
        *processCount = *processCount + 1;
    sem_post(processWrite);

		if(*numOfAllHydro < 2 || *numOfAllOxy < 1){
			*numOfAllHydro = 10000;
			for (int i = 0; i < 1000; ++i) {
				sem_post(oxy);
				sem_post(hydro);
			}
		}

   


    sem_wait(mutex2);
    *count = *count -1;
    if(*count == 0){
        sem_wait(barrier1);
        sem_post(barrier2);
    }
    sem_post(mutex2);

    sem_wait(barrier2);
    sem_post(barrier2);

    

}



//main
int main(int argc, char *argv[]){
    param_t params;
    int sleepTime = 0;           
    int sleepTimeBonding = 0;
    

    int paramExit = setParams(argc, argv, &params);
    if(paramExit == -1){
        fprintf(stderr, "Spatne zadany argumenty"); 
        exit(1);
    }
    setShaMem();
    *processCount = 1;

    fp = fopen("proj2.out", "w");
    if(fp == NULL){
        fprintf(stderr, "chyba pri zapisu do souboru");
        exit(1);
    }

    setbuf(fp, NULL);
    *fullMolecule = 3;
    *numOfAllHydro = params.NH;
    *numOfAllOxy = params.NO;

    int main_proccess = fork();

    if(main_proccess == 0){ //child

      for(int i = 0;i < params.NH ; i++){ 
      sleepTime = (random() % (params.TI + 1));
      usleep(sleepTime * 1000);
         if(fork() == 0)
        {   
            sleepTimeBonding = (random() % (params.TB + 1));
            //i = numeb of oxygen
            hydroGen(i, sleepTimeBonding);
            exit(0);
        }
    }

    }else if (main_proccess > 0){ //parent process
         
    for(int i = 0;i < params.NO ; i++){ 
    sleepTime = (random() % (params.TI + 1));
    usleep(sleepTime * 1000);
    if(fork() == 0)
        {
            sleepTimeBonding = (random() % (params.TB + 1));
            oxyGen(i, sleepTimeBonding );
            exit(0);
        }
    }
    }else{
        fprintf(stderr, "nastala chyba pri fork");
        exit(1);
    }
    
    for(int i=0;i < params.NH + params.NO ;i++) 
    wait(NULL);
    
    
    
    cleanup();
    exit(0);
    return 0;
}