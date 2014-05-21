#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>
#include "tracker.h"

#define MAX_ARG 100
#define SIZE 2048

#define REGEXANNOUNCE " *announce listen [0-9][0-9]* (seed)? \\[(([a-zA-Z0-9\\._-][a-zA-Z0-9\\._-]* [0-9][0-9]* [0-9][0-9]* [a-zA-Z0-9][a-zA-Z0-9]*)( ([a-zA-Z0-9\\._-][a-zA-Z0-9\\._-]* [0-9][0-9]* [0-9][0-9]* [a-zA-Z0-9][a-zA-Z0-9]*))*)?\\] (leech)? \\[(([a-zA-Z0-9][a-zA-Z0-9]*)( ([a-zA-Z0-9][a-zA-Z0-9]*))*)?\\] *"

#define REGEXGETFILE " *getfile ([a-zA-Z0-9][a-zA-Z0-9]*) *"

#define REGEXUPDATE " *update( seed \\[(([a-zA-Z0-9][a-zA-Z0-9]*)( ([a-zA-Z0-9][a-zA-Z0-9]*))*)?\\])?( leech \\[(([a-zA-Z0-9][a-zA-Z0-9]*)( ([a-zA-Z0-9][a-zA-Z0-9]*))*)?\\])? *"

#define REGEXLOOK " *look \\[((filename[=]\"[a-zA-Z0-9\\._-][a-zA-Z0-9\\._-]*)\"|(filesize[<>=]\"[0-9][0-9]*)\")( ((filename[=]\"[a-zA-Z0-9\\._-][a-zA-Z0-9\\._-]*)\"|(filesize[<>=]\"[0-9][0-9]*)\"))*\\] *"

char* ip;
int s_port;
active_connection client_args;

char* concat(char *s1, char *s2)
{
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  char *result = malloc(len1+len2+1);
  memcpy(result, s1, len1);
  memcpy(result+len1, s2, len2+1);
  return result;
}

char *str_sub (char *s, int start, int end)
{
  char *new_s = NULL;

  if (s != NULL && start < end)
    {
      new_s = malloc (sizeof (*new_s) * (end - start + 2));
      if (new_s != NULL)
  {
    int i;

    for (i = start; i <= end; i++)
      {
        new_s[i-start] = s[i];
      }
    new_s[i-start] = '\0';
  }
      else
  {
    fprintf (stderr, "Memoire insuffisante\n");
    exit (EXIT_FAILURE);
  }
    }
  return new_s;
}

void parse_end(){
  char* msg = "ok\n";
  printf("%s", msg);
  write(client_args->sock , msg , strlen(msg));
}

void parse_look(char** argv){
  char **argv_look = NULL;
  char *p = NULL;
  char delim[] = "\"[]";
  size_t i;
  char c;
  int reti;
  int pos;
  int tmp;
  int len;
  char *saisie;
  char *temp;
  char *name;
  long filesize;
  file_s file;
  int count = 0;
  int j = 0;

  TAILQ_FOREACH(file, &file_list_all, nextAll){
    count++;
  }
  int tabFileOk[count];
  for(j = 0; j < count; j++){
    tabFileOk[j] = 0;
  }
  argv_look = malloc(sizeof(char *) * MAX_ARG);
  for(pos = 1; pos < MAX_ARG; pos++){
    i = 0;
    tmp = strlen(argv[pos]);
    saisie = (char*) malloc(sizeof(char) * (tmp+1));
    strncpy(saisie, argv[pos], tmp);
    saisie[tmp - 1] = ' ';
    saisie[tmp] = '\0';

    if(NULL != (p = strrchr(saisie, ' ')))
      *p = '\0';

    temp = saisie;
    while ((p = strtok(temp, delim)) !=NULL)  {
      if(i < 100){
  argv_look[i] = malloc(sizeof(char) * (1+strlen(p)));
  strcpy(argv_look[i], p);
  i++;
      }
      else
  break;
      temp = NULL;
    }       
  
    argv_look[i] = NULL;
    argv_look[i+1] = '\0';
    
    TAILQ_FOREACH(file, &file_list_all, nextAll){
      printf("Log parse look : %s\n", file->name);
    }
    j = 0;
    len = strlen(argv_look[0]);
    if(argv_look[0][len - 1] == '='){
      if(strcmp(argv_look[0], "filename=") == 0){
  name = argv_look[1];
  TAILQ_FOREACH(file, &file_list_all, nextAll){
    if(strcmp(file->name, name) == 0){
      if(tabFileOk[j] != 2)
        tabFileOk[j] = 1;
    }
    else 
      tabFileOk[j] = 2;
    j++;
  }
      }
      else if(strcmp(argv_look[0], "filesize=") == 0){
  filesize = atol(argv_look[1]);
  TAILQ_FOREACH(file, &file_list_all, nextAll){
    if(file->t_length == filesize){
      if(tabFileOk[j] != 2)
        tabFileOk[j] = 1;
    }
    else
      tabFileOk[j] = 2;
    j++;
  }
      }
    }
    else if(argv_look[0][len-1] == '>'){
      filesize = atol(argv_look[1]);
      TAILQ_FOREACH(file, &file_list_all, nextAll){
  if(file->t_length > filesize){
    if(tabFileOk[j] != 2)
      tabFileOk[j] = 1;
  }
  else
    tabFileOk[j] = 2;
  j++;
      }
    }
    else if(argv_look[0][len-1] == '<'){
      filesize = atol(argv_look[1]);
      TAILQ_FOREACH(file, &file_list_all, nextAll){
  if(file->t_length < filesize){
    if(tabFileOk[j] != 2)
      tabFileOk[j] = 1;
  }
  else
    tabFileOk[j] = 2;
  j++;
      }
    }
    
    for(j = 0; argv_look[j] != NULL; j++)  { 
      free(argv_look[j]); 
    }
    if(argv[pos][tmp - 1] == ']')
      break;
  }
  free(argv_look);
  j = 0;
  char *tmpS;
  char *s;
  int isFirst = 0;
  char rep[256] = "list ";
  char list[256];
  strcat(rep, "[");
  tmpS = rep;
  TAILQ_FOREACH(file, &file_list_all, nextAll){
    if(tabFileOk[j] == 1){
      if(isFirst != 0){
  s = concat(tmpS, " ");
  tmpS = s;
      }
      sprintf(list, "%s %ld %ld %s", file->name, file->t_length, file->p_length, file->key);
      s = concat(tmpS, list);
      tmpS = s;
      isFirst = 1;
    }
    j++;
  }
  s = concat(tmpS, "]\n");
  printf("Log parse look response :%s\n", s);
  write(client_args->sock , s, strlen(s));
  free(s);
}

void parse_leech(int pos, client_s client, char** argv){
  
  int i;
  int num;
  int length;
  int ttl;
  int sl;
  char *name;
  char *key;
  long t_length;
  long p_length;
  int isAlreadyFile;
  file_s file;
  file_s new_file;

  /*Sensiblement le même traitement que pour seed, sauf que pas d'histoire de multiples 
    (car on a juste une liste de key)*/
  if(argv[pos] == NULL){
    parse_end();
  }
  else if(strcmp(argv[pos], "[]") == 0){
    parse_end();
  }
  else{
    i = pos;
    num = 0;
    for(i = pos; i < MAX_ARG; i++){
      length = strlen(argv[i]);
      if(argv[i][length - 1] == ']'){
        break;
      }
      num++;
    }
    for(i = 0; i <= num; i++){
      isAlreadyFile = 0;
      if(argv[pos + i][0] == '[' && argv[pos + i][strlen(argv[pos + i]) -1] == ']')
        key = str_sub(argv[pos + i], 1, strlen(argv[pos + i ]) -2);
      else if(argv[pos + i][0] == '[') 
        key = str_sub(argv[pos + i], 1, strlen(argv[pos + i]));
      else if(argv[pos + i][strlen(argv[pos + i])-1] == ']')
        key = str_sub(argv[pos + i], 0, strlen(argv[pos + i]) - 2);
      else
        key = str_sub(argv[pos + i], 0, strlen(argv[pos + i]));
      if(!(TAILQ_EMPTY(&(client->file_list)))){
        TAILQ_FOREACH(file, &(client->file_list), next){
          if(strcmp(file->key, key) == 0){
            isAlreadyFile = 1;
            break;
          }
        }
      }
      /*Si le fichier n'est pas déjà présent dans la liste du client, on récupère les informations de 
  ce leech dans la liste de tous les fichiers*/
      if(isAlreadyFile == 0){
        if(!(TAILQ_EMPTY(&file_list_all))){
          TAILQ_FOREACH(file, &file_list_all, nextAll){
            if(strcmp(key, file->key) == 0){
              name = file->name;
              t_length = file->t_length;
              p_length = file->p_length;
              ttl = TTL;
              sl = LEECH;
              new_file = malloc(sizeof(struct file_s));
              new_file->key = key;
              new_file->name = name;
              new_file->t_length = t_length;
              new_file->p_length = p_length;
              new_file->ttl = ttl;
              new_file->sl = sl;
              TAILQ_INSERT_TAIL(&(client->file_list), new_file, next);
              break;
            }
          }
        }
      }
      else 
        free(key);
    }
    parse_end();
  }
}


void parse_seed(int pos, client_s client, char** argv){
 
  int i;
  int num;
  int length;
  int ttl;
  int sl;
  int fileCount;
  int fileCountTmp;
  int count;
  char *name;
  char *key;
  long t_length;
  long p_length;
  int isAlreadyFile;
  file_s file;
  file_s new_file;

  /* Si on a [], on peut passer dans leech si il est présent, sinon end*/
  if(strcmp(argv[pos], "[]") == 0){
    if(argv[pos +1] != NULL){
      if(strcmp(argv[pos + 1], "leech") == 0){
        parse_leech(pos + 2, client, argv);
      }
    }
    else
      parse_end();
  }
  /* sinon, on teste d'abord le nombre d'arguments présents après le seed, qui doit être un multiple de 4*/
  else{
    i = pos;
    num = 0;
    for(i = pos; i < MAX_ARG; i++){
      length = strlen(argv[i]);
      if(argv[i][length - 1] == ']'){
        break;
      }
      num++;
    }
    count = 0;
    if((num+1)%4 != 0){
      char* msg = "Erreur de syntaxe\n";
      printf("%s\n", msg);
      write(client_args->sock, msg, strlen(msg));
    }

    /*Si c'est bon, on récupère name, les length et key*/
    else{
      for(i = 0; i <= num; i+=4){
        count +=4;
        isAlreadyFile = 0;
        if(argv[pos + i][0] == '[') 
          name = str_sub(argv[pos + i], 1, strlen(argv[pos + i]));
        else
          name = str_sub(argv[pos + i], 0, strlen(argv[pos + i]));
        t_length = atol(argv[pos + i+1]);
        p_length = atol(argv[pos + i+2]);
        ttl = TTL;
        sl = SEED;
        if(argv[pos + i + 3][strlen(argv[pos + i + 3])-1] == ']')
          key = str_sub(argv[pos + i+3], 0, strlen(argv[pos + i+3]) - 2);
        else
          key = str_sub(argv[pos + i + 3], 0 , strlen(argv[pos + i + 3]));
        
        /*Si une key est déjà présente dans la liste des fichiers, on ne fait rien*/
        if(!(TAILQ_EMPTY(&(client->file_list)))){
          TAILQ_FOREACH(file, &(client->file_list), next){
            if(strcmp(file->key, key) == 0){
              isAlreadyFile = 1;
              break;
            }
          }
        }
        /*Sinon, on ajoute un nouveau fichier*/
        if(isAlreadyFile == 0){
          new_file = malloc(sizeof(struct file_s));
          new_file->key = key;
          new_file->name = name;
          new_file->t_length = t_length;
          new_file->p_length = p_length;
          new_file->ttl = ttl;
          new_file->sl = sl;
          TAILQ_INSERT_TAIL(&(client->file_list), new_file, next);
          fileCount = 0;
          fileCountTmp = 0;

          /*Si le fichier n'est pas déjà présent dans la liste de tous les fichiers
            (donc pas déjà ajouté par un autre client), on l'ajoute*/
          TAILQ_FOREACH(file, &file_list_all, nextAll){
            if(strcmp(file->key, new_file->key) != 0)
              fileCountTmp++;
            fileCount++;
          }
          if(fileCount == fileCountTmp){
            TAILQ_INSERT_TAIL(&file_list_all, new_file, nextAll);
          }
        }
      }

      pos += count;
      if(strcmp(argv[pos], "leech") == 0){
        parse_leech(pos + 1, client, argv);
      }
      else
        parse_end();
    }
  }
}

void parse_getfile(char** argv){

  char *key;
  file_s file;
  client_s client;
  char *s;
  char *tmp;
  char ipPort[256];
  int isFirst = 0;
  
  //TAILQ_HEAD(,client_s) client_tmp;
  //TAILQ_INIT(&client_tmp);
  if(argv[1] != NULL){
    key = argv[1];
    char rep[256] = "peers ";
    strcat(rep, key);
    strcat(rep, " [");
    tmp = rep;
    if(!(TAILQ_EMPTY(&client_list))){
      TAILQ_FOREACH(client, &client_list, next){
        TAILQ_FOREACH(file, &(client->file_list), next){
          if(strcmp(file->key, key) == 0){
            if(isFirst != 0){
              s = concat(tmp, " ");
              tmp = s;
            }
            sprintf(ipPort, "%s:%d", client->ip, client->port);
            s = concat(tmp, ipPort);
            tmp = s;
            isFirst = 1;
            break;
          }
        }
      }
    }
    s = concat(tmp, "]\n");
    printf("Log getfile : %s\n", s);
    write(client_args->sock , s, strlen(s));
    free(s);
  }
  else {
    char* msg = "Erreur de syntaxe\n";
    printf("Log getfile : %s\n", msg);
    write(client_args->sock, msg, strlen(msg));
  }
}

void parse_leech_update(int pos, client_s client, char** argv){
  int i;
  int num;
  int length;
  int ttl;
  int sl;
  char *name;
  char *key;
  long t_length;
  long p_length;
  int isAlreadyFile;
  file_s file,tempF;
  file_s fileC, tempCF;
  file_s new_file;
  client_s tmp;
  int isStillFile;
  
  if(argv[pos] == NULL){
    parse_end();
  }

  /*Même fonctionnement que pour la fonction seed_update.*/
  else if(strcmp(argv[pos], "[]") == 0){
    if(!(TAILQ_EMPTY(&(client->file_list)))){
      TAILQ_FOREACH_SAFE(file, &(client->file_list), next, tempF){
        if(file->sl == LEECH){
          TAILQ_REMOVE(&(client->file_list), file, next);
          isStillFile = 0;
          TAILQ_FOREACH(tmp, &client_list, next){
            if((strcmp(tmp->ip, client->ip)!=0) || tmp->sys_port != client->sys_port){ //MODIFIER AVEC LE PORT AUSSI
              TAILQ_FOREACH_SAFE(fileC, &(tmp->file_list), next, tempCF){
                if(strcmp(fileC->key, file->key) == 0){
                  isStillFile = 1;
                  break;
                }
              }
            }
            if(isStillFile == 1)
              break;
          }
          if(isStillFile == 0){
            TAILQ_REMOVE(&file_list_all, file, nextAll);
          }
        }
      }
    }
    parse_end();
  }
  else{
    TAILQ_FOREACH(file, &(client->file_list), next){
      if(file->sl == LEECH)
        file->sl = UNDEF;
    }
    i = pos;
    num = 0;
    for(i = pos; i < MAX_ARG; i++){
      length = strlen(argv[i]);
      if(argv[i][length - 1] == ']'){
        break;
      }
      num++;
    }
    for(i = 0; i <= num; i++){
      isAlreadyFile = 0;
      if(argv[pos + i][0] == '[') 
        key = str_sub(argv[pos + i], 1, strlen(argv[pos + i]));
      else if(argv[pos + i][strlen(argv[pos + i])-1] == ']')
        key = str_sub(argv[pos + i], 0, strlen(argv[pos + i]) - 2);
      else
        key = str_sub(argv[pos + i], 0, strlen(argv[pos + i]));;
      if(!(TAILQ_EMPTY(&(client->file_list)))){
        TAILQ_FOREACH(file, &(client->file_list), next){
          if(strcmp(file->key, key) == 0){
            isAlreadyFile = 1;
            file->ttl = TTL;
            file->sl = LEECH;
            break;
          }
        }
      }
      if(isAlreadyFile == 0){
        if(!(TAILQ_EMPTY(&file_list_all))){
          TAILQ_FOREACH(file, &file_list_all, nextAll){
            if(strcmp(key, file->key) == 0){
              name = file->name;
              t_length = file->t_length;
              p_length = file->p_length;
              ttl = TTL;
              sl = LEECH;
              new_file = malloc(sizeof(struct file_s));
              new_file->key = key;
              new_file->name = name;
              new_file->t_length = t_length;
              new_file->p_length = p_length;
              new_file->ttl = ttl;
              new_file->sl = sl;
              TAILQ_INSERT_TAIL(&(client->file_list), new_file, next);
              break;
            }
          }
        }
      }
    }
    pos += num +1;
    TAILQ_FOREACH(file, &(client->file_list), next){
      if(file->sl == UNDEF){
        TAILQ_REMOVE(&(client->file_list), file, next);
        isStillFile = 0;
        TAILQ_FOREACH(tmp, &client_list, next){
          if((strcmp(tmp->ip, client->ip)!=0) || tmp->sys_port != client->sys_port){ //MODIFIER AVEC LE PORT AUSSI
            TAILQ_FOREACH(fileC, &(tmp->file_list), next){
              if(strcmp(fileC->key, file->key) == 0){
                isStillFile = 1;
                break;
              }
            }
          }
          if(isStillFile == 1)
            break;
        }
        if(isStillFile == 0){
          TAILQ_REMOVE(&file_list_all, file, nextAll);
          free(file->name);
          free(file->key);
          free(file);
        }
      }
    }
    parse_end();
  }
}


void parse_seed_update(int pos, client_s client, char** argv){
  int i;
  int num;
  int length;
  int ttl;
  int sl;
  char *name;
  char *key;
  long t_length;
  long p_length;
  int isAlreadyFile;
  file_s file, tempF;
  file_s fileC, tempCF;
  file_s new_file;
  client_s tmp;
  int isStillFile;
  printf("Log client_ip : %s\n", client->ip);
  
  /*Les fonctions seed_update et leech_update font sensiblement la même chose (il faudrait regrouper le code 
    pour que ce soit plus propre)*/
  if(argv[pos] == NULL){
    parse_end();
  }

  /*Si on a [], on va supprimer tous les fichiers de la liste du client, et on va regarder si le fichier est
    encore seed par un autre client. S'il ne l'est pas, on va le supprimer de la liste de tous les fichiers.*/
  else if(strcmp(argv[pos], "[]") == 0){
    TAILQ_FOREACH_SAFE(file, &(client->file_list), next, tempF){
      if(file->sl == SEED){
        TAILQ_REMOVE(&(client->file_list), file, next);
        isStillFile = 0;
        TAILQ_FOREACH(tmp, &client_list, next){
          if((strcmp(tmp->ip, client->ip)!=0) || tmp->sys_port != client->sys_port){ //MODIFIER AVEC LE PORT AUSSI
            TAILQ_FOREACH_SAFE(fileC, &(tmp->file_list), next, tempCF){
              if(strcmp(fileC->key, file->key) == 0){
                isStillFile = 1;
                break;
              }
            }
          }
          if(isStillFile == 1)
            break;
        }
        if(isStillFile == 0){
          TAILQ_REMOVE(&file_list_all, file, nextAll);
          free(file->name);
          free(file->key);
          free(file);
        }
      }
    }
    if(strcmp(argv[pos + 1], "leech") == 0){
      parse_leech_update(pos + 2, client, argv);
    }
    else
      parse_end();
  }

  /*Sinon, on va passer tous les fichiers seed en UNDEF, puis on va récupérer toutes les key de la liste.
    Ensuite, on va tester si les key correspondent à des fichiers seed par le client. Si oui, on va repasser les 
    fichiers en seed. Sinon, on va ajouter les fichiers en récupérant les informations manquantes à partir 
    de la liste de tous les fichiers.*/
  else{
    TAILQ_FOREACH(file, &(client->file_list), next){
      if(file->sl == SEED)
       file->sl = UNDEF;
    }
    i = pos;
    num = 0;
    for(i = pos; i < MAX_ARG; i++){
      length = strlen(argv[i]);
      if(argv[i][length - 1] == ']'){
        break;
      }
      num++;
    }
    for(i = 0; i <= num; i++){
      isAlreadyFile = 0;
      if(argv[pos  +i][0] == '[' && argv[pos + i][strlen(argv[pos+i]) -1] == ']')
        key = str_sub(argv[pos + i], 1, strlen(argv[pos + i]) - 2);
      else if(argv[pos + i][0] == '[') 
        key = str_sub(argv[pos + i], 1, strlen(argv[pos + i]));
      else if(argv[pos + i][strlen(argv[pos + i])-1] == ']')
        key = str_sub(argv[pos + i], 0, strlen(argv[pos + i]) - 2);
      else
        key = str_sub(argv[pos + i], 0, strlen(argv[pos + i]));
      
      printf("Log seed update key : %s\n", key);
      if(!(TAILQ_EMPTY(&(client->file_list)))){
        TAILQ_FOREACH(file, &(client->file_list), next){
          if(strcmp(file->key, key) == 0){
            isAlreadyFile = 1;
            file->ttl = TTL;
            file->sl = SEED;
            break;
          }
        }
      }
      if(isAlreadyFile == 0){
        if(!(TAILQ_EMPTY(&file_list_all))){
          TAILQ_FOREACH(file, &file_list_all, nextAll){
            if(strcmp(key, file->key) == 0){
              name = file->name;
              t_length = file->t_length;
              p_length = file->p_length;
              ttl = TTL;
              sl = SEED;
              new_file = malloc(sizeof(struct file_s));
              new_file->key = key;
              new_file->name = name;
              new_file->t_length = t_length;
              new_file->p_length = p_length;
              new_file->ttl = ttl;
              new_file->sl = sl;
              TAILQ_INSERT_TAIL(&(client->file_list), new_file, next);
              break;
            }
          }
        }
      }
      else
        free(key);
    }
    pos += num +1;

    /*Si des fichiers sont encore en UNDEF, on va les supprimer de la liste du client, puis on va regarder si 
      ces fichiers sont encore seed par d'autres clients. Si plus personne ne seed, on les supprimer de la liste
      de tous les fichiers.*/
    TAILQ_FOREACH_SAFE(file, &(client->file_list), next, tempF){
      if(file->sl == UNDEF){
        TAILQ_REMOVE(&(client->file_list), file, next);
        isStillFile = 0;
        TAILQ_FOREACH(tmp, &client_list, next){
          if((strcmp(tmp->ip, client->ip)!=0) || tmp->sys_port != client->sys_port){ //MODIFIER AVEC LE PORT AUSSI
            TAILQ_FOREACH(fileC, &(tmp->file_list), next){
              if(strcmp(fileC->key, file->key) == 0){
                isStillFile = 1;
                break;
              }
            }
          }
          if(isStillFile == 1)
            break;
        }
        if(isStillFile == 0){
          TAILQ_REMOVE(&file_list_all, file, nextAll);
          free(file->name);
          free(file->key);
          free(file);
        }
      }
    }
    if(strcmp(argv[pos], "leech") == 0){
      parse_leech_update(pos + 1, client, argv);
    }
    else
      parse_end();
  }
}

void parse_update(char** argv){
  
  client_s tmp;
  client_s client = NULL;

  /*On récupère le client qui veut faire un update grâce à son ip*/
  TAILQ_FOREACH(tmp, &client_list, next){
    if((strcmp(client_args->ip, tmp->ip)==0) && client_args->sys_port == tmp->sys_port){ //MODIFIER AVEC LE PORT AUSSI
      client = tmp;
      printf("Log update : %d %s\n", client->sys_port, client->ip);
      break;
    }
  }
  if(client != NULL){
    if(argv[1] != NULL){
      if(strcmp(argv[1], "seed")==0){
  parse_seed_update(2, client, argv);
      }
      else if(strcmp(argv[1], "leech")==0){
  parse_leech_update(2,client,argv);
      }
      else {
  char* msg = "Erreur de syntaxe\n";
  printf("%s\n", msg);
  write(client_args->sock, msg, strlen(msg));
      }
    }
    else {
      char* msg = "Erreur de syntaxe\n";
      printf("%s\n", msg);
      write(client_args->sock, msg, strlen(msg));
    }
  }
  else {
    char* msg = "Client inexistant\n";
    printf("%s\n", msg);
    write(client_args->sock, msg, strlen(msg));
  }
}

void parse_announce(char** argv){
  
  client_s client;
  client_s tmp;
  int isAlreadyClient;
  // client->ip = client_args->ip;
  //client->port = atoi(argv[1]);
  
  if ((!argv[1]) || (argv[1] && !argv[2])){
    char* error_msg = "Erreur de syntaxe\n";
    printf("%s\n", error_msg);
    write(client_args->sock , error_msg , strlen(error_msg));
    return;
  }

  /*On teste si la deuxième chaîne est bien listen, sinon ko*/
  if(strcmp(argv[1], "listen") == 0){
    client = malloc(sizeof(struct client_s));
    client->ip = malloc(sizeof(char) * (strlen(client_args->ip) +1));
    strncpy(client->ip, client_args->ip, strlen(client_args->ip));
    client->ip[strlen(client_args->ip)] = '\0'; //RAJOUTER LE PORT DE CONNEXION
    /* client->port = malloc(sizeof(char) * (strlen(argv[2]) +1)); */
    /* strncpy(client->port, argv[2], strlen(argv[2])); */
    /* client->port[strlen(argv[2])] = '\0'; */
    // Verification de l'argument
    client->port = atoi(argv[2]);
    client->sys_port = client_args->sys_port;
    isAlreadyClient = 0;

    /*On vérifie que ce n'est pas déjà un client qui refait un announce, sinon ko*/
    if(!(TAILQ_EMPTY(&client_list))){
      TAILQ_FOREACH(tmp, &client_list, next){
        if((strcmp(tmp->ip, client->ip) == 0) && tmp->sys_port == client->sys_port){
          isAlreadyClient = 1;
        }
      }
    }
    if(isAlreadyClient == 0){
      TAILQ_INSERT_TAIL(&client_list, client, next);
      TAILQ_INIT(&(client->file_list));    

      if(argv[3] != NULL){
        if(strcmp(argv[3], "seed") == 0 || strcmp(argv[3], "seed ") == 0){
          parse_seed(4,client, argv);
        }
        else if(strcmp(argv[3], "leech") == 0 || strcmp(argv[3], "leech ") == 0){
          parse_leech(4,client, argv);
        }
  else if(strcmp(argv[3], "leech") == 0 || strcmp(argv[3], "leech ") == 0){
    parse_leech(4,client, argv);
  }
      }
    } else {
      free(client->ip);
      free(client);
      char* msg = "Erreur de syntaxe\n";
      printf("%s\n", msg);
      write(client_args->sock, msg, strlen(msg));
    }
  }
  else{
    //free(client);
    char* msg = "Erreur de syntaxe\n";
    printf("%s\n", msg);
    write(client_args->sock, msg, strlen(msg));
  }
  //printf("\nClient ip : %s, port : %d\n", client->ip, client->port);

}
int parse_regex(char* saisieInit, char* func){
  
  regex_t regex;
  int reti;
  char msgbuf[100];

  int tmp = strlen(saisieInit);
  char *saisie = (char*) malloc(sizeof(char) * (tmp+1));
  strncpy(saisie, saisieInit, tmp);
  saisie[tmp - 1] = ' ';
  saisie[tmp] = '\0';

  reti = regcomp(&regex, func, REG_NOSUB | REG_EXTENDED);
  printf("plante\n");
  if(reti!=0){
    printf("Impossible d'exécuter la regex\n");
    reti = -1;
  }
  else{
    reti = regexec(&regex, saisie, 0, NULL, 0);
    if(reti==0){printf("Ok regex\n");}
    else if(reti == REG_NOMATCH){
      char* msg = "Erreur de syntaxe\n";
      printf("%s\n", msg);
      write(client_args->sock, msg, strlen(msg));
      reti = -1;
    }
    else{
      regerror(reti, & regex, msgbuf, sizeof(msgbuf));
      printf("La regex a échoué : %s\n", msgbuf);
      reti = -1;
    }
  }
  regfree(&regex);
  free(saisie);
  return reti;
}

void parse(char* saisie, char* saisieInit){

  char **argv = NULL;
  char *p = NULL;
  char delim[] = " ";
  size_t i = 0;
  char c;
  int reti;

  /*Ce passage permet de découper la chaîne en plusieurs chaînes stockées dans argv,
    grâce aux espaces*/
  if(NULL != (p = strrchr(saisie, ' ')))
    *p = '\0';
  else
    while(' ' != (c = fgetc(stdin)) && c != EOF);

  argv = malloc(sizeof(char *) * MAX_ARG);
  char *temp = saisie;
  //int count = 0;
  while ((p = strtok(temp, delim)) !=NULL)  {
    if(i < MAX_ARG){
      argv[i] = malloc(sizeof(char) * (1+strlen(p)));
      strcpy(argv[i], p);
      i++;
    }
    else
      break;
    temp = NULL;
  }       
  
  argv[i] = NULL;
  argv[i+1] = '\0';
   
  if(strcmp(argv[0], "announce") == 0){
    reti = parse_regex(saisieInit, REGEXANNOUNCE);
    if(reti==0)
      parse_announce(argv);
  }
  else if(strcmp(argv[0], "look") == 0){
    reti = parse_regex(saisieInit, REGEXLOOK);
    if(reti==0)
      parse_look(argv);
  }
  else if(strcmp(argv[0], "getfile") == 0){
    reti = parse_regex(saisieInit, REGEXGETFILE);
    if(reti==0)
      parse_getfile(argv);
  }
  else if(strcmp(argv[0], "update") == 0){
    reti = parse_regex(saisieInit, REGEXUPDATE);
    if(reti==0)
      parse_update(argv);
  }
  else{
    char* msg = "Erreur de syntaxe\n";
    printf("%s\n", msg);
    write(client_args->sock, msg, strlen(msg));
  }
  for(i = 0; argv[i] != NULL; i++)   { 
    printf("arg[%zu] = %s ", i, argv[i]);
    free(argv[i]); 
  }
  printf("\n");
  free(argv);
}

void init(active_connection args, char* saisieInit){
  
    int tmp = strlen(saisieInit);
  if(tmp > 1){
    char *saisie = (char*) malloc(sizeof(char) * (tmp+1));
    strncpy(saisie, saisieInit, tmp);
    saisie[tmp - 1] = ' ';
    saisie[tmp] = '\0';
    printf("%s\n", saisie);
    
    client_args = args;
      
    parse(saisie, saisieInit);
    free(saisie);
    //free(ip);
    //free(saisie);
    //printf("Coucou\n");
  }
  else{
    char* msg = "Erreur de syntaxe\n";
    printf("%s\n", msg);
    write(client_args->sock, msg, strlen(msg));
  }
}
