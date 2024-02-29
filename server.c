#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <time.h>
#include <utmp.h> //pentru utmp si timpul
#define client_server_read "CLIENT->SERVER.txt"
#define client_server_write "SERVER->CLIENT.txt"

struct Mesaj{
    int length; 
    char data[512]; 
};


void stergespatii(char* linie) {
       int inceput=0, final=strlen(linie)-1 , n=strlen(linie), i;
   for(i=0; i<=n; i++) // stergem spatiile de inainte
   {
       if(isspace(linie[i])){ //nu functioneaza fara functia isspace.
          inceput++;     
       }
       else{break;}
   }
   for(i=strlen(linie)-1; i>=inceput; --i) //stergem spatiile incepand din spate spre fata.
   {
       if(isspace(linie[i])){
           final--;
       }else{break;}
   }
   if(inceput>0 || final<strlen(linie)-1) //punem conditia asta in cazul in care nu avem spatii nici inainte nici dupa. ca sa nu puna iar \0
   {
   memmove(linie , linie + inceput , final - inceput + 1);
   linie[final - inceput + 1]= '\0';
   }
}

int main(){

    struct Mesaj username;
    mkfifo(client_server_write, 0666);
    mkfifo(client_server_read, 0666);

        //partea de socket Parinte<->copil1 
        int sockp[2], pid; 
   if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) < 0) 
            { 
              perror("Eroare la socket ul dintre Parinte si Copil1\n"); 
              exit(1); 
            }

    if((pid=fork())==-1){
        perror("Eroare la fork , Copil1");
        exit(2);
    }else if(pid == 0)
    {
        //copil1   
        close(sockp[1]); //inchdem capatul parinte->socket
        struct Mesaj msg;

        FILE* fdConfig=fopen("Config.txt","r");
        int logout=1; //setam logout true
        
        //facem pipe copil2->copil1
           int fd[2]; //pipe copil2 -> copil1l
          if(pipe(fd) == -1)
            {
                perror("Eroare la crearea pipe copil2->copil1");
                exit(-1);
            }
        
        //facem pipe copil3->copi1        
            int pipecop3cop1[2];
           if(pipe(pipecop3cop1) == -1)
            {
                perror("Eroare la crearea pipe copil3->copil1");
                exit(-1);
            }
                
         int bytes;   
         int status;    
        while(1)
        {     
            if((bytes=read(sockp[0],&msg,sizeof(struct Mesaj)))<0){
                perror("[copil]Eroare la read din socket Parinte-Copil1");
                exit(-1);
            }

             msg.data[strcspn(msg.data, "\n")] = '\0';
             printf("%s ",msg.data);
                //partea de logare/delogare
            if(logout)
            {    
                char linie[256];
                //mutam cursorul la inceput:
                fseek(fdConfig,0,SEEK_SET);
              //printf("? %ld",strlen(msg.data));
              if(strlen(msg.data)>=1)  //verificam cazul in care dai enter fara sa scrii absolut nimic
              {
                while(fgets(linie,sizeof(linie),fdConfig)){
                    stergespatii(linie);  //sterg eventualele spatii nedorite      
                  
                    if(strcmp(linie,msg.data)==0){
                        printf("Copil: Yes\n");
                        logout=0;       //nu mai suntem logout
                        strcpy(msg.data,"Yes");
                    }
                }
              }
            }
            else if(strcmp(msg.data,"logout") == 0){
                logout=1; // suntem logut
                strcpy(msg.data,"No");
            }
            else if(strcmp(msg.data,"get-logged-users")==0){ //"get-logged-users"
                //aici procesam get-loged-users
                pid_t pid2;

                if((pid2=fork())==-1){
                    perror("Eroare la fork ,copil2");
                    exit(-2);
                }
                else if(pid2==0){
                    //copil2
                    //inchidem socket,nu folosim
                     close(sockp[1]);
                     close(sockp[0]);
                     close(fd[0]);
                     close(pipecop3cop1[1]);
                     close(pipecop3cop1[0]);
                    struct utmp *utmp;
                    time_t timpul_de_intrare;

                    setutent(); //setez pointerul *utmp la inceputl fisierului utmp.

                     //struct Mesaj msj;
                     char msj[512];
                     struct Mesaj mesajstruct;
                     strcpy(mesajstruct.data,"");
                    while((utmp=getutent())!=NULL){ //getutent este o functie care ia informatill le prelucreaza si le afiseaza intr un format mai frumos si usor de descifrat

                        timpul_de_intrare = utmp->ut_tv.tv_sec;
                           // Convertirea timpului
                        struct tm *timpl_local = localtime(&timpul_de_intrare);

                       sprintf(msj,"Username : %s \nHostname : %s\nTime entry : %02d:%02d:%02d \n",
                       utmp->ut_user , utmp->ut_host,timpl_local->tm_hour,timpl_local->tm_min,timpl_local->tm_sec); //%02d afiseaza doar primele 2 cifre,in caz ca este unn numar de o cifra va afisa 0 in fata.
                       strcat(mesajstruct.data,msj);

                    }
                     endutent(); //se inchidem fisierul utmp


                    printf("Inainte sa scriu in pipe copil2->copil1\n %s ",mesajstruct.data);
                   if(write(fd[1],&mesajstruct,sizeof(struct Mesaj))<0){
                        perror("Eroare la scriere in pipe ,copil2");
                        exit(-2);
                     }
                    close(fd[1]); //inchdem pipe ul copil2->copil1
                    exit(2);
                }
                //parinte / copil1
                //asteptam sa se termine copilul2
                struct Mesaj buff;
                wait(NULL); 
                if(read(fd[0],&buff,sizeof(struct Mesaj))<0){
                       perror("Eroare la citirea din pipe copil2->copil1\n");
                    }
                buff.length=strlen(buff.data);
               

                //transferam datele din buff citi din pipe in msg principal.
                strcpy(msg.data,buff.data);
            }
            else if(strncmp(msg.data,"get-proc-info",13)==0){ //"get-proc-info : pid"
                //aici procesam comanda get-proc-info : pid
                pid_t pid3;
     

                //vrem sa extragem pidul;
                char pidbuff[256];
                int pidclient;
                int i=0,j=0;
                
                while(i<strlen(msg.data)){   
                    if(isdigit(msg.data[i])){
                        pidbuff[j++]=msg.data[i];
                    }
                    i++;
                }
                pidbuff[j]='\0';
                pidclient=atoi(pidbuff); //stransofrmam din char in int

                //printf("Pid din CHR in int %d\n",pidclient);

                if((pid3=fork())==-1){
                    perror("Eroare la fork,copil3");
                }
                else if(pid3==0){
                   //tot ce i mostenit si nu folosesc
                    close(sockp[1]);
                    close(sockp[0]);
                    close(fd[0]);
                    close(fd[1]);
                    close(pipecop3cop1[0]);
                   
                    //copil3
                    

                     //cream un fisier
                    FILE *fisier_proc;
                    char proc_copil1[512];
                    //construiesc calea spre fisier 
                    sprintf(proc_copil1,"/proc/%d/status",pidclient);
                    printf("Pidul clientului %d \n",pidclient);

                    //declar variabilele de care urmeaza sa ne folosim
                    char buff[512];
                    struct Mesaj all;
                    strcpy(all.data,"");

                    int nr=0;

                  if((fisier_proc=fopen(proc_copil1,"r"))==NULL){
                    perror("Nu a gasit pidul");
                    strcpy(all.data,"Nu s-a gasit pidul");
                  }
                  else 
                  { //din proc putem in general doar citi     
                
                    while(fgets(buff,sizeof(buff),fisier_proc)>0 && nr!=5){

                       if(strstr(buff,"Name:")!=NULL) //ca am gasit Subsirui Name in buff,afiseaza pozitia de la care l a gasit,probabil 0;
                       {
                        printf("%s",buff);
                        strcat(all.data,buff); 
                        nr++;
                       }
                       else if(strstr(buff,"State:")!=NULL)
                       {
                        printf("%s",buff);
                        strcat(all.data,buff); 
                        nr++;
                       }
                       else if(strstr(buff,"PPid:")!=NULL)
                       {
                        printf("%s",buff);
                        strcat(all.data,buff); 
                        nr++;
                       }
                       else if(strstr(buff,"Uid:")!=NULL)
                       {
                        printf("%s",buff);
                        strcat(all.data,buff); 
                        nr++;
                       }
                       else if(strstr(buff,"VmSize:")!=NULL)
                       {
                        printf("%s",buff);
                        strcat(all.data,buff); 
                        nr++;
                       }

                    }
                
                      fclose(fisier_proc);

                      printf("%s",all.data); //afisam informatiile despre pid
                  }
                  
                    if(write(pipecop3cop1[1],&all,sizeof(struct Mesaj))<0){
                        perror("Eroare la scrierea in sockcop3cop1\n");
                        exit(-1);
                    }
                   close(pipecop3cop1[1]);
                   exit(1);
                }
              
                //parinte          
                if(wait(&status)==-1){
                    perror("Eroare in copil 3 \n");
                } 
                //astept copilul
                
                //citesc informatiile despre pid din socket
                if(read(pipecop3cop1[0],&msg,sizeof(struct Mesaj))<0){
                    perror("Eroare la citirea din sockcop3cop1\n");
                        exit(-1);
                }
                
            }                
                   //printf("Sunt inainte de scrierea in socket, %s \n" , msg.data);
            //pun lungimea lui msg:
            msg.length=strlen(msg.data);
            //scriu spre parinte
            if(write(sockp[0],&msg,sizeof(struct Mesaj))<0){
                 perror("[copil]Error in write");
                exit(-1);
               }
            
        }
        close(fd[0]);
        close(fd[1]);
        close(sockp[0]);
        fclose(fdConfig);
        close(pipecop3cop1[1]);
        close(pipecop3cop1[0]);
        
    }
      //parinte

      sleep(1);
    
    close(sockp[0]); 

    int fdww,fdr;
    char buff[256];

     fdr=open(client_server_read,O_RDONLY); 
     fdww=open(client_server_write,O_WRONLY);

    while(1){

        if(read(fdr,&username,sizeof(struct Mesaj))<0){
            perror("eror");
            exit(1);
         }
        printf("Parinte %s\n",username.data);
    

               
             if(write(sockp[1],&username,sizeof(struct Mesaj))<0){
            perror("[parinte]Error in write");
            exit(1);
             }
            
            if(read(sockp[1],&username,sizeof(struct Mesaj))<0){
                 perror("[parinte]Erroare la read");
                 exit(1);
             }
              
            if(write(fdww ,&username,sizeof(struct Mesaj))<0){
                 perror("error");
                 exit(2);
             }
            
    }
    close(sockp[1]);
    close(fdr);
    close(fdww);
    
    
    return 0;
}


