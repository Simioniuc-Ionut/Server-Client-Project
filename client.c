#include <sys/types.h>
#include <sys/socket.h> //biblioteca de ssocketuri

#include <netinet/in.h>
#include <netdb.h>

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
//struct pt mesaj


#define client_server_write "CLIENT->SERVER.txt"
#define client_server_read "SERVER->CLIENT.txt"

struct Mesaj{
    int length; 
    char data[512]; 
};

int main(){

    int fdw,fdr;
    char buff[256];
    struct Mesaj username;
    struct Mesaj rd;
   
    fdw=open(client_server_write,O_WRONLY);

    int login=0;

   while(1)
   { 
   
    if(login==0){
          printf("login : username \n");
          fgets(username.data, sizeof(username.data), stdin);
    }
    else{
         printf("\nIntroduceti comenzi/mesaje\n|get-logged-users|get-proc-info : pid|logout|quit|\n");
         fgets(username.data, sizeof(username.data), stdin);
        }

      if(write(fdw,&username,sizeof(struct Mesaj))<0){
          perror("Eroare la scrierea in CLIENT->SERVER.txt\n");
          exit(1);
       }
         printf("a scris %s\n",username.data);
         fdr=open(client_server_read,O_RDONLY);
      if(read(fdr,&rd,sizeof(struct Mesaj))<0){
          perror("Eroare la citirea din SERVER->CLIENT.txt\n");
          exit(2);
      }

      printf("a primit : %d%s \n",rd.length,rd.data);
      rd.data[strcspn(rd.data, "\n")] = '\0'; //cu strcspn gasesc terminatorul endline si l inlocuiesc pt a putea sa se realizeze compararile corect
         //partea de logare/delogare/quit
      if(strcmp(rd.data,"quit") == 0 ){break;}
      else if(strcmp(rd.data,"Yes")==0){
          printf("\nS-a logat cu succes\n");
         login=1;


      }
      else if(strcmp(rd.data,"No") == 0){
         //ne am delogat.
          printf("\nS-a delogat cu succes \n");
          login=0;
      }
      else if(login != 1)
     {
         printf("\nNu s-a putut realiza logarea \n");
     }
   }
    printf("V-ati deconectat!\n\n");
    close(fdr);
    close(fdw);
    return 0;
}