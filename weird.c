#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "myfilesystem.h"


#include <pthread.h>
#define true 1
#define false 0

typedef struct HelpStruct{
   // these files are fixed in size. they will not run out of space
   FILE *file_data;
   FILE *directory_table;
   FILE *hash_data;
   int dir_size;
   int file_size;
   int threads_max;
   int threads;
}Help;

typedef struct LinkedList{
    int data;
    int offset;
    int length;
    struct LinkedList *next;
}Node;



//pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; DON'T USE THIS. THIS IS NOT RECURSIVE

// this is the first function run. initialise all data structures needed.
// Return a pointer to a memory area (void *) that you can use to store data for
// the virtual file system. The pointer will be void * helper.
// f1 is the filename of the file_data file.
// f2 is the filename of the directory table file.
// f3 is filename of the hash_data file.
// Make sure to create these files if they don't exist.

// Set up threads. I guess I can only go up to n_processors.

void * init_fs(char * f1, char * f2, char * f3, int n_processors) {
   //set up the files. If they don't exist, it will be created
   struct HelpStruct * helper = (struct HelpStruct*) malloc(sizeof(struct HelpStruct));
   helper->file_data = fopen(f1,"rb+");
   helper->directory_table = fopen(f2,"rb+");
   //FILE* file1 = fopen(f1,"rb+");
   //FILE* file2 = fopen(f2,"rb+");
   if (helper->file_data==NULL){
      // printf("Unable to open file1!\n");
   }
   if (helper->directory_table==NULL){
      // printf("Unable to open file2!\n");
   }
   helper->hash_data = fopen(f3, "rb+");
   char abc[1]= {'1'};
   fread(abc,1,1,((Help*)helper)->directory_table);
   fseek(helper->directory_table,0,SEEK_END);
   int len = ftell(helper->directory_table);
   helper->dir_size = len;
   fseek(helper->file_data,0,SEEK_END);
   len = ftell(helper->file_data);
   helper->file_size = len;
   fseek(helper->directory_table,0,SEEK_SET);
   helper->threads_max = n_processors;
   helper->threads = 0;



   return helper;
}

void close_fs(void * helper) {
   fclose(((Help*)helper)->file_data);
   fclose(((Help*)helper)->directory_table);
   fclose(((Help*)helper)->hash_data);
   free(helper);
}


void truncate(char* filename,char truncated[]){
   for(int i = 0; i<63; i++){
      if(strlen(filename)>i){
         truncated[i]=filename[i];
      }else{
         truncated[i]='\0';
      }
   }
   truncated[63]='\0';
}


/*
HUGE ISSUE


The stuff I am reading in is really dodgy.
*/



int create_file(char * filename, size_t length, void * helper) {
   FILE* dir = ((Help*)helper)->directory_table;
   unsigned char buf[((Help*)helper)->dir_size];
   unsigned char buf2[((Help*)helper)->file_size];
   fseek(((Help*)helper)->directory_table,0,SEEK_SET);
   fseek(((Help*)helper)->file_data,0,SEEK_SET);
   fread(buf,sizeof(char),((Help*)helper)->dir_size,((Help*)helper)->directory_table);
   fread(buf2,sizeof(char),((Help*)helper)->file_size,((Help*)helper)->file_data);
   char truncated[64];
   truncate(filename,truncated);
   //create a char buf of that one.
   //check if the filename already exists
   char compared[((Help*)helper)->dir_size];
   for(int i = 0; i< ((Help*)helper)->dir_size; i++){
      compared[i]=buf[i];
   }
   for(int i = 0; i< ((Help*)helper)->dir_size;i+=72){
      if(strcmp(&(compared[i]),truncated)==0){
         fseek(((Help*)helper)->directory_table,0,SEEK_SET);
         unsigned char test[0];
         fread(&test,sizeof(char),1,((Help*)helper)->directory_table);
         return 1;
      }
   }
   //check an empty space in directory
   int success = -1;
   for(int i = 0; i< ((Help*)helper)->dir_size; i+=72){
      if(i==0){
         //success = -1;
         fseek(((Help*)helper)->directory_table,0,SEEK_SET);
      }else{
         //success = -1;
         fseek(((Help*)helper)->directory_table,72,SEEK_CUR);
      }
      unsigned char test[1];
      //test = fgets(test, 1,((Help*)helper)->directory_table);
      //fgtes causes a fatal error
      fread(test, 1, 1,((Help*)helper)->directory_table );
      if(test[0]=='\0'&&success==-1){
         success = i;
      }
   }
   fseek(((Help*)helper)->directory_table,0,SEEK_SET);
   //right now, we will ahve found an empty space if sucess is not -1.

   if(success == -1){ // if it failed, then there isn't enough space in the directory. Therefore,
      // printf("why?\n");
      return 2;

   }
   // printf("looking ofr space\n");
   // printf("%x \n",buf[64]);
   // printf("%x \n",buf[65]);
   // printf("%x \n",buf[66]);
   // printf("%x \n",buf[67]);
   // printf("%u \n",(int)buf[64]+((int)buf[65]<<8)+((int)buf[66]<<16)+((int)buf[67]<<24));
   int exists[((Help*)helper)->file_size];
   for(int i = 0; i<((Help*)helper)->file_size; i++){
      exists[i]=false;
   }
   for(int i = 0; i< (((Help*)helper)->dir_size); i+=72){
      //check if this code is an actual file or an empty/deleted space.
      fseek(((Help*)helper)->directory_table,i,SEEK_SET);//start of each block
      unsigned char value[1];
      fread(&value[0],1,1,((Help*)helper)->directory_table);
      if(value[0]!='\0'){
         // get offset bytes into offsetA
         fseek(((Help*)helper)->directory_table,i+64,SEEK_SET);
         int trueOffset = -1;
         fread(&trueOffset,4,1,((Help*)helper)->directory_table);
         int trueLength = -1;
         fseek(((Help*)helper)->directory_table,i+68,SEEK_SET);
         fread(&trueLength,4,1,((Help*)helper)->directory_table);
         int counter = trueOffset;

         // if((trueOffset+trueLength)>((Help*)helper)->dir_size){
         //    printf("BIG ERROR\n");
         // }
         while(counter<trueOffset+trueLength){
            exists[counter]=true;
            counter++;
         }
      }
   }
   fseek(((Help*)helper)->directory_table,0,SEEK_SET);

   //NOW, it can be assumed that we found a spot in the directory file.
   // Now, it is time to find an area inside of it that has enough space for lengthA
   int position = -1;
   int counter = 0;
   for(int i = 0; i < ((Help*)helper)->file_size;i++){
      if(counter != length){
         if(exists[i]==false){
            if(position ==-1){
               position = i;
            }
            counter++;
         }else{
            position = -1;
            counter = 0;
         }
      }
   }

      // printf("counter is %d, and length is %d\n",counter, (int)length);
   //now, if counter == length, the value of position will be
   if(counter!=length){
      // repack
      //hold on, what will reapck actually do here?
      repack(helper);
      // printf("repack happened\n");

      // recalculate exists

      for(int i = 0; i<((Help*)helper)->file_size; i++){
         exists[i]=false;
      }
      for(int i = 0; i< (((Help*)helper)->dir_size); i+=72){
         fseek(((Help*)helper)->directory_table,i,SEEK_SET);//start of each block
         unsigned char value[1];
         fread(&value[0],1,1,((Help*)helper)->directory_table);
         if(value[0]!='\0'){
            fseek(((Help*)helper)->directory_table,i+64,SEEK_SET);
            int trueOffset = -1;
            fread(&trueOffset,4,1,((Help*)helper)->directory_table);
            int trueLength = -1;
            fseek(((Help*)helper)->directory_table,i+68,SEEK_SET);
            fread(&trueLength,4,1,((Help*)helper)->directory_table);
            int count = trueOffset;
            // if((trueOffset+trueLength)>((Help*)helper)->dir_size){
            //    printf("BIG ERROR\n");
            // }
            while(count<trueOffset+trueLength){
               exists[count]=true;
               count++;
            }
         }
      }

      // recalculate position and counter.
      //position is the position of where the file being created will be
      // int position = -1;
      // int counter = 0;
      position = -1;
      counter = 0; // I'm pretty sure that using int just makes a copy with limited scope
      for(int i = 0; i < ((Help*)helper)->file_size;i++){
         if(counter != length){
            if(exists[i]==false){
               if(position ==-1){
                  position = i;
               }
               counter++;
            }else{
               position = -1;
               counter = 0;
            }
         }
      }
   }

   if(counter == length){
      if(position == -1){
         // printf("here?\n");
         compute_hash_tree(helper);
         return 2;
      }
      // place this inside of the directory_table
      fseek(dir,success,SEEK_SET);
      fwrite(&truncated,64,1,dir);
      fseek(((Help*)helper)->directory_table,success+64,SEEK_SET);
      fwrite(&position,4,1,dir);
      fseek(((Help*)helper)->directory_table,success+68,SEEK_SET);
      fwrite(&length,4,1,dir);
      fseek(((Help*)helper)->directory_table,0,SEEK_SET);

      // zero out the stuff inside of the file
      fseek(((Help*)helper)->file_data,position,SEEK_SET);
      unsigned char zero[length];
      for(int i = 0; i< length; i++){
         zero[i]='\0';
      }
      fwrite(&zero,length,1,((Help*)helper)->file_data);
      fseek(((Help*)helper)->file_data,0,SEEK_SET);
   }else{
      // printf("counter is %d, and legnth is %d\n",counter, (int)length);
      compute_hash_tree(helper);
      return 2;
      // return 0;
   }
   fseek(((Help*)helper)->directory_table,0,SEEK_SET);
   unsigned char test[0];
   fread(&test,sizeof(char),1,((Help*)helper)->directory_table);
   compute_hash_tree(helper);
   return 0;
}

/*
dir
files
Loop through to the correct filename.
store the position in int position.
if resizing it to be greater :
   loop though dir. If any have a offset that is between the current value's offset and offset+length,
   remove the original file.
   repack();
   create_file();
if it is resizing to be smaller :
   read the file into a unsigned char array. Turn the back into '\0'.
   loop though files
   to the correct position
   fwrite it in.
*/

int bothCharSame(char a[],char b[]){
   int same = true;
   for(int i = 0; i< 64; i++){
      if(a[i]!=b[i]){
         same = false;
      }
   }
   return same;
}


int resize_file(char * filename, size_t length, void * helper){
   if(length>((Help*)helper)->file_size){
      return 2;
   }
   FILE * dir = ((Help*)helper)->directory_table;
   FILE * files = ((Help*)helper)->file_data;
   int size = ((Help*)helper)->dir_size/72;
   char truncated[64];
   truncate(filename,truncated);
   int position = -1;
   int valueLength = -1;
   int valueOffset = -1;
   // unsigned char values[length];
   for(int i = 0; i< size; i++){
      int buf[1];
      char name[64];
      fseek(dir,72*i,SEEK_SET);
      fread(&name,64,1,dir);
      if(bothCharSame(name, truncated)==true){
         position = i;
         fseek(dir,72*i+64,SEEK_SET);
         fread(&buf,4,1,dir);
         valueOffset = buf[0];
         fseek(dir,72*i+68,SEEK_SET);
         fread(&buf,4,1,dir);
         valueLength = buf[0];
      }
   }
   unsigned char values[valueLength];
   fseek(files,valueOffset,SEEK_SET);
   fread(&values,valueLength,1,files);
   if(position == -1){
      return 1;
   }
   // now, position will be the number of 72-byte blocks in this went.
   // values, valueOffset, and valueLength will have the correct value.
   int tes[4];
   fseek(dir,72*position+64,SEEK_SET);
   fread(&tes,4,1,dir);
   fseek(dir,72*position+68,SEEK_SET);
   fread(&tes,4,1,dir);
   if(length > valueLength){ // increasing the length
      int repackbool = false;
      for(int i = 0; i<size; i++){ // check for VALUE
         int o;
         int l;
         fseek(dir,72*i+64,SEEK_SET);
         fread(&o,4,1,dir);
         fseek(dir,72*i+68,SEEK_SET);
         fread(&l,4,1,dir);
         if(o>=valueOffset&&o<valueOffset+length){ // between the offset and the new length
            repackbool=true;
         }
      }

      if(repackbool==true){
         //delete the current file
         fseek(dir,position*72,SEEK_SET);
         unsigned char n = '\0';
         fwrite(&n,1,1,dir);
         //REPACK
         repack(helper); //position variable is unchanged
         //find the position in file_data
         int currentMax = 0;
         for(int j =0; j<size;j++){
            int buf1[1];
            int buf2[1];
            int bufv[1];
            fseek(dir,72*j,SEEK_SET);
            fread(&bufv,1,1,dir);
            fseek(dir,72*j+64,SEEK_SET);
            fread(&buf1,4,1,dir);
            fseek(dir,72*j+68,SEEK_SET);
            fread(&buf2,4,1,dir);
            if(buf1[0]+buf2[0] > currentMax && bufv[0]!='\0'){
               currentMax = buf1[0]+buf2[0];
            }
         }
         //currentMax will thus be the position that one can add in. // check to make sure it is within bound
         if(currentMax+length>((Help*)helper)->file_size){
            compute_hash_tree(helper);
            return 2;
         }
         // go to the position, place all the values in.
         fseek(dir,72*position,SEEK_SET);
         fwrite(&truncated,64,1,dir);
         fseek(dir,72*position+64,SEEK_SET);
         fwrite(&currentMax,4,1,dir);
         fseek(dir,72*position+68,SEEK_SET);
         fwrite(&length,4,1,dir);
         // place the values into file_data
         fseek(files,currentMax,SEEK_SET);
         unsigned char buf[length];
         for(int i=0;i<length;i++){
            if(i<valueLength){
               buf[i]=values[i];
            }else{
               buf[i]='\0';
            }
         }
         fwrite(&buf,length,1,files);
      }else{ //if repack was not called
         // go to the position, place all the values in.
         fseek(dir,72*position,SEEK_SET);
         fwrite(&truncated,64,1,dir);
         fseek(dir,72*position+64,SEEK_SET);
         fwrite(&valueOffset,4,1,dir);
         fseek(dir,72*position+68,SEEK_SET);
         fwrite(&length,4,1,dir);
         // place the values into file_data
         unsigned char buf[length];
         for(int i=0;i<length;i++){
            if(i<valueLength){
               buf[i]=values[i];
            }else{
               buf[i]='\0';
            }
         }
         fwrite(&buf,length,1,files);
      }
   }else if(length < valueLength){
      // go to the position, place all the values in.
      fseek(dir,72*position,SEEK_SET);
      fwrite(&truncated,64,1,dir);
      fseek(dir,72*position+64,SEEK_SET);
      fwrite(&valueOffset,4,1,dir);
      fseek(dir,72*position+68,SEEK_SET);
      fwrite(&length,4,1,dir);
      // place the values into file_data
      unsigned char buf[length];
      for(int i=0;i<length;i++){
         buf[i]=values[i];
      }
      fseek(files,valueOffset,SEEK_SET);
      fwrite(&buf,length,1,files);

   }
   fseek(files,54,SEEK_SET);
   unsigned char test[1];
   fread(&test,1,1,files);
   compute_hash_tree(helper);
   return 0;
}

void repack(void * helper) {
   FILE* dir = ((Help*)helper)->directory_table;
   FILE* files = ((Help*)helper)->file_data;
   // unsigned char test[1];
   // fseek(dir,64,SEEK_SET);
   // fread(&test,1,1,dir);
   // fseek(dir,68,SEEK_SET);
   // fread(&test,1,1,dir);
   // fseek(dir,0,SEEK_SET);
   // fseek(files,0,SEEK_SET);
   int size = (((Help*)helper)->dir_size)/72;
   // REMEMBER TO FREE NODES
   Node *head = (Node*)malloc(sizeof(Node));
   Node *curr = head;
   head->data = -1;
   head->offset = -1;
   head->length = -1;
   head->next = NULL;
   for(int i = 0; i< size; i++){
      unsigned char doesFileExist[1];
      fseek(dir, 72*i,SEEK_SET);
      fread(&doesFileExist,1,1,dir);
      if(doesFileExist[0]!='\0'){
         Node * new = (Node*)malloc(sizeof(Node));
         // fseek(dir,72*i,SEEK_SET);
         // fread(&(new->data),4,1,dir);
         // the above two lines make no sense
         new->data = 72*i;
         fseek(dir,72*i+64,SEEK_SET);
         fread(&(new->offset),4,1,dir);
         fseek(dir,72*i+68,SEEK_SET);
         fread(&(new->length),4,1,dir);
         curr = head;
         int j = 0; // j will be the position of curr, with 0 as the head.
         while(curr!=NULL&&(new->offset > curr-> offset)){
            curr = curr->next;
            j++;
         }
         // right now, j will be the value that new will replace, shifting the original j to new->next
         //0 : head, 1: head->next ,...... j-1: something, j: new, j+1 : original j
         Node*curr = head;
         for(int k = 1; k<j; k++){ // won't run if j is 1. Thus, curr is still head in that case.
            curr = curr->next;
         }
         new->next = curr->next;
         curr->next = new;
      }

   }
   // Now, I have an ordered list of nodes.
   curr = head->next; // cos head isn't actuall one of the files
   int minpos = 0;
   //int end = 0;
   while(curr!=NULL){
      if(minpos > curr->offset && curr!=head){
      }else if(minpos < curr->offset){ // i.e. no need to do anything when the minimum and the offset are the same
         unsigned char values[curr->length];
         fseek(files,(curr->offset),SEEK_SET);
         fread(&values,curr->length,1,files);
         fseek(files,minpos,SEEK_SET);
         fwrite(&values,curr->length,1,files);
         fseek(dir,(curr->data)+64,SEEK_SET); // this somehow edits 68
         fwrite(&minpos,4,1,dir); // places the minimum value into the directory_table's offset
         minpos = minpos+(curr->length);
      }else if(minpos == curr->offset){
         minpos = minpos+(curr->length);
      }
      curr = curr->next;
   }
   // free Nodes
   curr = head;
   Node* after = head->next;
   while(after!=NULL){
      free(curr);
      curr = after;
      after = after->next;
   }
   free(curr);
   compute_hash_tree(helper);
}

int delete_file(char * filename, void * helper) {
   FILE * dir = ((Help*)helper)->directory_table;
   // FILE * files = ((Help*)helper)->file_data;
   int size = ((Help*)helper)->dir_size/72;
   char truncated[64];
   truncate(filename,truncated);
   for(int i = 0; i< size; i++){
      int buf[1]={'\0'};
      char name[64];
      fseek(dir,72*i,SEEK_SET);
      fread(&name,64,1,dir);
      if(bothCharSame(name, truncated)==true){
         fseek(dir,72*i,SEEK_SET);
         fwrite(&buf,1,1,dir);
         return 0;
      }
   }
   return 1;
}

int rename_file(char * oldname, char * newname, void * helper) {
   FILE * dir = ((Help*)helper)->directory_table;
   // FILE * files = ((Help*)helper)->file_data;
   int size = ((Help*)helper)->dir_size/72;
   char truncated[64];
   truncate(newname,truncated);
   for(int i = 0; i< size; i++){
      char name[64];
      fseek(dir,72*i,SEEK_SET);
      fread(&name,64,1,dir);
      if(bothCharSame(name, truncated)==true){
         return 1;
      }
   }
   truncate(oldname,truncated);
   for(int i = 0; i< size; i++){
      char name[64];
      fseek(dir,72*i,SEEK_SET);
      fread(&name,64,1,dir);
      if(bothCharSame(name, truncated)==true){
         truncate(newname,truncated);
         fseek(dir,72*i,SEEK_SET);
         fwrite(truncated,64,1,dir);
         return 0;
      }
   }
   return 1;
}

int read_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
   FILE * dir = ((Help*)helper)->directory_table;
   FILE * files = ((Help*)helper)->file_data;
   FILE * hash = ((Help*)helper)->hash_data;
   int filesize = ((Help*)helper)->file_size;
   int blocknumber = filesize/256;
   int blocksize = 2*blocknumber-1;
   int size = ((Help*)helper)->dir_size/72;
   char truncated[64];
   truncate(filename,truncated);
   for(int i = 0; i< size; i++){
      char name[64];
      fseek(dir,72*i,SEEK_SET);
      fread(&name,64,1,dir);
      if(bothCharSame(name, truncated)==true){
         int fileOffset;
         int fileLength;
         fseek(dir,72*i+64,SEEK_SET);
         fread(&fileOffset,4,1,dir);
         fseek(dir,72*i+68,SEEK_SET);
         fread(&fileLength,4,1,dir);
         // printf("count %ld, offset %ld, fileLength %d\n",count, offset, fileLength[0]);
         if(count+offset>=fileLength||(fileOffset+offset+count)>((Help*)helper)->file_size){
            return 2;
         }
         fseek(files,fileOffset+offset,SEEK_SET);
         char test[count];
         fread(&test,count,1,files);

         //check for hash
         int min = (fileOffset+offset)/256;
         if(min%2==1){
            min = min-1;
         }
         int blocks = ceil((fileOffset+offset+count)/256.0);
         // if((fileOffset+offset+count)%256==0){
         //    blocks = (fileOffset+offset+count)/256;
         // }else{
         //    blocks = (fileOffset+offset+count)/256 + 1;
         // }
         // printf("the file's blocks are %d\n",blocknumber);
         for(int b=min; b<blocks;b+=2){
            int pos = blocksize-blocknumber+b;

            uint8_t tmp[256];
            uint8_t str[16];
            fseek(files,(b)*256,SEEK_SET);
            fread(tmp,1,256,files);
            fletcher(tmp,256,str);
            // uint8_t new[256];
            while(pos>0){
               fseek(hash,pos*16,SEEK_SET);
               uint8_t hashV[16];
               fread(hashV,1,16,hash);
               // for(int j=0;j<16;j++){
               //    printf("%d",hashV[j]);
               // }
               // printf("\n");
               // for(int j=0;j<16;j++){
               //    printf("%d",str[j]);
               // }
               // printf("\n");
               if(memcmp(hashV,str,16)!=0){
                  return 3;
               }else{
                  // printf("OK!\n");
               }
               // fseek(hash, pos*16,SEEK_SET);
               // fwrite(str,1,16,hash);

               fseek(hash, pos*16,SEEK_SET);
               uint8_t buf[32];
               fread(buf,1,32,hash);
               fletcher(buf,32,str);
               pos = (pos-1)/2;
            }
         }



         //end of check


         // buf = (void*) test; doesn't work
         memcpy(buf, test, count);
         return 0;
      }
   }
   return 1;
}

int write_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
   char test[count];
   memcpy(test, buf, count);
   FILE * dir = ((Help*)helper)->directory_table;
   FILE * files = ((Help*)helper)->file_data;
   unsigned char t[1];
   fseek(dir,208,SEEK_SET);
   fread(&t,1,1,dir);
   int size = ((Help*)helper)->dir_size/72;
   char truncated[64];
   truncate(filename,truncated);
   int repackyes = false;
   int exists = false;
   for(int i = 0; i< size; i++){
      char name[64];
      fseek(dir,72*i,SEEK_SET);
      fread(&name,64,1,dir);
      if(bothCharSame(name, truncated)==true){
         exists = true;
         unsigned int fileOffset;
         unsigned int fileLength;
         fseek(dir,72*i+64,SEEK_SET);
         fread(&fileOffset,4,1,dir);
         fseek(dir,72*i+68,SEEK_SET);
         fread(&fileLength,4,1,dir);
         if(count+offset>=fileLength){ // repack
            if(offset>=fileLength){ // I assume that returning 2 prevents it from actually affecting any other data
               return 2;
            }else{
               repackyes=true;
            }
         }
         if(repackyes==false){
            fseek(files,fileOffset+offset,SEEK_SET);
            fwrite(&test,count,1,files);
            fseek(dir,208,SEEK_SET);
            fread(&test,1,1,dir);
            compute_hash_tree(helper);
            return 0;
         }
      }
   }
   if(exists==false){
      return 1;
   }
   if(repackyes==true){
      int check = resize_file(filename,count+offset,helper);
      // resize_file(truncated,count+offset,helper);
      // printf("resize happened\n");
      // printf("%d\n",check);
      if(check!=0){
         compute_hash_tree(helper);
         return 3;
      }
      for(int i = 0; i< size; i++){
         char name[64];
         fseek(dir,72*i,SEEK_SET);
         fread(&name,64,1,dir);
         // printf("name is %s, truncated is %s\n",name, truncated);
         if(bothCharSame(name, truncated)==true){
            // printf("got it! the name is %s\n",name);
            exists = true;
            int fileOffset;
            int fileLength;
            fseek(dir,72*i+64,SEEK_SET);
            fread(&fileOffset,4,1,dir);
            fseek(dir,72*i+68,SEEK_SET);
            fread(&fileLength,4,1,dir);
            // printf("The new fileOffset is %d, new fileLength is %d\n", fileOffset, fileLength);
            char test[count];
            memcpy(test, buf, count);
            fseek(files,fileOffset+offset,SEEK_SET);
            fwrite(&test,count,1,files);
            fseek(dir,208,SEEK_SET);
            fread(&test,1,1,dir);
            // printf("at byte 208, the byte is %d\n",test[0]);
            compute_hash_tree(helper);
            return 0;
         }
      }
   }

   compute_hash_tree(helper);
   return 1;
}

ssize_t file_size(char * filename, void * helper) {
   FILE * dir = ((Help*)helper)->directory_table;
   int size = ((Help*)helper)->dir_size/72;
   char truncated[64];
   truncate(filename,truncated);
   for(int i = 0; i< size; i++){
      char name[64];
      fseek(dir,72*i,SEEK_SET);
      fread(&name,64,1,dir);
      if(bothCharSame(name, truncated)==true){
         int fileLength;
         fseek(dir,72*i+68,SEEK_SET);
         fread(&fileLength,4,1,dir);
         return fileLength;
      }
   }
   return -1;
}

void fletcher(uint8_t * buf, size_t length, uint8_t * output) {
   // if(length%4!=0){
   //    for(int i=length;i<16;i++){
   //       *(buf+i) = 0;
   //    }
   // }
   uint64_t a = 0; //It needs to be 64 bytes so that a+data[i] doesn't go all the way around. before the mod is applied
   uint64_t b = 0;
   uint64_t c = 0;
   uint64_t d = 0;
   for(int i = 0; i<length; i+=4){
      uint32_t value = buf[i]+(buf[i+1]<<8)+(buf[i+2]<<16)+(buf[i+3]<<24);
      a = (a + value)%4294967295;
      // a = (uint32_t)a;
      b = (b + a)%4294967295;
      // b = (uint32_t)b;
      c = (c + b)%4294967295;
      // c = (uint32_t)c;
      d = (d + c)%4294967295;
      // d = (uint32_t)d;
      // printf("a is now %d\n",(int) a);

   }
   uint8_t * hash_value = malloc(16);
   for(int i = 0; i<4;i++){
      *(hash_value+i) = (uint8_t)((unsigned char*)&a)[i];
   }
   for(int i = 4; i<8;i++){
      *(hash_value+i) = (uint8_t)((unsigned char*)&b)[i-4];
   }
   for(int i = 8; i<12;i++){
      *(hash_value+i) = (uint8_t)((unsigned char*)&c)[i-8];
   }
   for(int i = 12; i<16;i++){
      *(hash_value+i) = (uint8_t)((unsigned char*)&d)[i-12];
   }

   for(int i =0; i<16;i++){
      *(output+i) = *(hash_value+i);
   }
   free(hash_value);

}

/*go through 25 bytes. Count the number of. The number of levels is log(that number ) with base 2.
 go through each level from the bottom, up.

*/
void compute_hash_tree(void * helper) {
   FILE * files = ((Help*)helper)->file_data;
   FILE * hash = ((Help*)helper)->hash_data;
   int filesize = ((Help*)helper)->file_size;
   int blocknumber = filesize/256;
   int size = 2*blocknumber-1;
   uint8_t arr[16*size];
   for(int i = 0; i< blocknumber;i++){
      uint8_t tmp[256];
      uint8_t str[16];
      fseek(files,256*i,SEEK_SET);
      fread(tmp,1,256,files);
      fletcher(tmp,256,str);
      for(int j =0; j<16;j++){
         // arr[16*size-16*blocknumber+16*i+j]=str[j];
         arr[16*(size-blocknumber+i)+j] = str[j];
      }
   }

   // int parent = size-blocknumber;
   for(int i = size-1; i>0; i-=2){
      uint8_t tmp[32];
      for(int j = 0 ; j< 16 ; j++){
         tmp[j] = arr[16*(i-1)+j];
         tmp[j+16] = arr[16*(i)+j];
      }
      uint8_t sixteen[16];
      fletcher(tmp,32,sixteen);
      for(int j = 0; j<16; j++){
         arr[16*((i-2)/2)+j] = sixteen[j];
      }
   }




   // uint8_t tmp[0];
   // fseek(hash,0,SEEK_SET);
   // fread(tmp,1,1,hash);
   // printf("%d\n",tmp[0]);
   fseek(hash,0,SEEK_SET);
   fwrite(arr,1,16*size,hash);

   // fseek(hash,0,SEEK_SET);
   // fread(tmp,1,1,hash);
   // printf("%d\n",tmp[0]);
}

void compute_hash_block(size_t block_offset, void * helper) {
   FILE * files = ((Help*)helper)->file_data;
   FILE * hash = ((Help*)helper)->hash_data;
   int filesize = ((Help*)helper)->file_size;
   int blocknumber = filesize/256;
   int size = 2*blocknumber-1;

   int pos = size-blocknumber+block_offset;
   if(block_offset%2==1){
      pos = pos-1;
   }
   uint8_t tmp[256];
   uint8_t str[16];
   fseek(files,block_offset*256,SEEK_SET);
   fread(tmp,1,256,files);
   fletcher(tmp,256,str);
   // uint8_t new[256];
   while(pos>=0){
      fseek(hash, pos*16,SEEK_SET);
      uint8_t buf[32];
      fread(buf,1,32,hash);
      fseek(hash, pos*16,SEEK_SET);
      fwrite(str,1,16,hash);
      fletcher(buf,32,str);
      if(pos==0){
         break;
      }else{
         pos =(pos-1)/2;
      }
   }

}
