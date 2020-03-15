#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "defn.h"
#include "AM.h"
#include "bf.h"
#include "math.h"
#include "Stack.h"

int AM_errno = AME_OK;

file open_files[MAX_OPEN_FILES];      // Πίνακας ανοιχτών αρχείων
scan open_scans[MAX_OPEN_SCANS];      // Πίνακας ανοιχτών σαρώσεων

bool TypeCheck(char attrType,               //Συνάρτηση που ελέγχει αν ο τύπος και το μέγεθος που έχουν δωθεί συμβαδίζουν
                 int attrLength){
  if(attrType == FLOAT || attrType == INTEGER)
  {
    if(attrLength != 4)
    {
      return false;
    }
  }
  else if(attrType == STRING)
  {
    if(attrLength < 1 || attrLength > 255)
    {
      return false;
    }    
  }
  else
  {
      return false;
  }  
  return true;
}


int Comparison(int fileDesc, void* value, void* Key)                        //Συνάρτηση για την σύγκριση της τιμής του κλειδιού με την τιμή του value
{
  if(open_files[fileDesc].attrType1 == INTEGER)                             //Εαν το κλειδί είναι τύπου INTEGER
  {
    if((*(int *)value) < (*(int *)Key))
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else if(open_files[fileDesc].attrType1 == FLOAT)                          //Εαν το κλειδί είναι FLOAT
  {
    if((*(float*)value) < (*(float *)Key))
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else if(open_files[fileDesc].attrType1 == STRING)                         //Εαν το κλειδί είναι STRING
  {
    if(strcmp((char *)value, (char *)Key) <  0)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    printf("No valid key type.\n");
    return 0;
  }
}

int FindFirstDataBlock(int root, int fd, int fileDesc)            //Επιστρέφει τον αριθμό του πρώτου block δεδομένων
{
  BF_ErrorCode bferror;
  BF_Block *block;
  blocktype bt;
  indexblock index;
  char *metadata, *metaptr;
  int next_block;

  BF_Block_Init(&block);

  
  if(bferror = BF_GetBlock(fd,root,block) != BF_OK)               //Παίρνουμε το block της ρίζας
  {
    BF_PrintError(bferror);
    return -1;
  }

  metadata = BF_Block_GetData(block);                             //Παίρνουμε τα δεδομένα του block της ρίζας
  memcpy(&bt,metadata,sizeof(blocktype));                       

  if(bt.DataBlock)                                                //Αν η ρίζα είναι block δεδομένων υπάρχει λάθος
  {
    return -1;
  }

  while(!bt.DataBlock)
  {
    metaptr = metadata;
    metaptr += sizeof(blocktype);                                  //Προσπέραση των πληροφοριών του blocktype
    memcpy(&index,metaptr,sizeof(indexblock));    
    next_block = index.Pointer;

    if(bferror = BF_UnpinBlock(block) != BF_OK)
    {
      BF_PrintError(bferror);
      return -1;
    }

    if(bferror = BF_GetBlock(fd,next_block,block) != BF_OK)               //Παίρνουμε το νεό block
    {
      BF_PrintError(bferror);
      return -1;
    }

    metadata = BF_Block_GetData(block);                             //Παίρνουμε τα δεδομένα του νέου block
    memcpy(&bt,metadata,sizeof(blocktype));         
  }

  if(bferror = BF_UnpinBlock(block) != BF_OK)
  {
    BF_PrintError(bferror);
    return -1;
  }

  BF_Block_Destroy(&block);

  return next_block;
}


int SearchData(int root, void *value, int fd, int fileDesc, stack *stackptr)  //Επιστρέφει το block δεδομένων που περιέχει την τιμή value 
{
  BF_Block *block;
  BF_ErrorCode bferror;
  blocktype bt;
  indexblock index;
  char *metadata, *metaptr;
  int key, counter, next_block;


  BF_Block_Init(&block);                                            //Αρχικοποίηση Block
  if(bferror = BF_GetBlock(fd,root,block) != BF_OK)                 //Παίρνουμε το Block της ριζας
  {
    BF_PrintError(bferror);
    return -2;
  }

  metadata = BF_Block_GetData(block);                              //Παίρνουμε τα δεδομένα του Block της ρίζας
  memcpy(&bt,metadata,sizeof(blocktype));

  if(bferror = BF_UnpinBlock(block) != BF_OK)
  {
    BF_PrintError(bferror);
    return -2;
  }

  if(bt.DataBlock)                                                  //Αν πρόκειται για Block δεδομένων, έχει συμβεί κάποιο σφάλμα
  {
    return -1;
  }

  if(bt.NumberOfStructs == 0)                                       //Περίπτωση στην οποία στο δέντρο υπάρχει μόνο η άδεια ρίζα
  {
    return -1;
  }

  next_block = root;
  while(!bt.DataBlock)                                              //Αναζήτηση του value στο B+-Tree
  {
    if(stackptr != NULL)
    {
      Push(next_block,stackptr);
    }
    metaptr = metadata;
    metaptr += sizeof(blocktype);                                  //Προσπέραση των πληροφοριών του blocktype
    memcpy(&index,metaptr,sizeof(indexblock));
    counter = 0;
    while(counter < bt.NumberOfStructs)                             //Αναζήτηση του value στο τρέχον Block
    {
      if(Comparison(fileDesc,value,index.Key))                      //Η περίπτωση που το value είναι μικρότερο από το πρώτο key, άρα ο κατάλληλος pointer είναι ο πρώτος του Block
      {
        next_block = index.Pointer;
        break;
      }
      else                                                          //Η περίπτωση που το value δεν είναι όυτε στην αρχή του Βlock, όυτε στο τέλος του Block 
      {
        if(counter == (bt.NumberOfStructs-1))                      //Η περίπτωση που το value είναι στο τέλος του Block, άρα ο κατάλληλος pointer είναι ο τελευταίος του Block
        {
          metaptr += (((BF_BLOCK_SIZE - sizeof(blocktype) - sizeof(int)) / sizeof(indexblock)) - (bt.NumberOfStructs - 1)) * sizeof(indexblock);
          memcpy(&next_block,metaptr,sizeof(int));
          break; 
        }
        metaptr += sizeof(indexblock);
        memcpy(&index,metaptr,sizeof(indexblock));
        if(Comparison(fileDesc,value,index.Key))
        {
          next_block = index.Pointer;
          break;
        }
        counter++;
      }
    }
    
    if(bferror = BF_UnpinBlock(block) != BF_OK)
    {
      BF_PrintError(bferror);
      return -2;
    }

    if(bferror = BF_GetBlock(fd,next_block,block) != BF_OK)
    {
      BF_PrintError(bferror);
      return -2;
    }
    metadata = BF_Block_GetData(block);                         //Παίρνω τα δεδομένα του next_block
    memcpy(&bt,metadata,sizeof(blocktype));
  }

  if(bferror = BF_UnpinBlock(block) != BF_OK)
  {
    BF_PrintError(bferror);
    return -2;
  }

  BF_Block_Destroy(&block);

  if(bt.DataBlock)
  {
    return next_block;                                            //Επιστρέφω το επιθυμητό Block δεδομένων
  }
  else
  {
    return -1;
  }
}

void FindLessValues(int scanDesc, char *metadata)                  //Αποθηκεύει τις εγγραφές με κλειδί μικρότερο απο ένα value
{
  int counter = 0;
  char *metaptr;
  char *temprecptr;
  datablock datastruct;
  datablockpointer datablockptr;

  metaptr = metadata + sizeof(blocktype);
  memcpy(&datastruct,metaptr,sizeof(datablock));
  temprecptr = open_scans[scanDesc].recs;                         //Δείκτης στην αρχή του buffer recs
  while(counter < open_scans[scanDesc].maxrecords || datastruct.data1 != NULL)    //Όσο υπάρχουν έγκυρες εγγραφές
  {
    if(open_files[open_scans[scanDesc].fileDesc].attrType1 == INTEGER)            //Ανάλογα με τον τύπο του κλειδιού
    {
      if( (*(int *)datastruct.data1) < (*(int *)open_scans[scanDesc].value))      //Αν η τιμή είναι μικρότερη απο αυτη του value
      {
        memcpy(temprecptr,&datastruct,sizeof(datablock));                         //Εγγραφή της τιμής στο recs
        temprecptr += sizeof(datablock);                                          //Προχωράμε τον temprecptr
        open_scans[scanDesc].reccounter++;                                        //Αυξάνουμε τις εγγραφές γι αυτό το block
      }
    }
    if(open_files[open_scans[scanDesc].fileDesc].attrType1 == FLOAT)
    {
      if( (*(float *)datastruct.data1) < (*(float *)open_scans[scanDesc].value))
      {
        memcpy(temprecptr,&datastruct,sizeof(datablock));
        temprecptr += sizeof(datablock);
        open_scans[scanDesc].reccounter++;
      }
    }
    if(open_files[open_scans[scanDesc].fileDesc].attrType1 == STRING)
    {
      if(strcmp((char *)datastruct.data1 , (char *)open_scans[scanDesc].value) < 0)
      {
        memcpy(temprecptr,&datastruct,sizeof(datablock));
        temprecptr += sizeof(datablock);
        open_scans[scanDesc].reccounter++;
      }
    }
    metaptr += sizeof(datablock);
    memcpy(&datastruct,metaptr,sizeof(datablock));
    counter++;
  }
  memcpy(&datablockptr,metaptr,sizeof(datablockpointer));
  open_scans[scanDesc].next_block = datablockptr.NextDataBlock;                   //Η μεταβλητή next_block παίρνει την τιμή του επόμενου block δεδομένων
  return;
}


void FindGreaterValues(int scanDesc, char *metadata)              //Αποθηκεύει τις εγγραφές με κλειδί μεγαλύτερο απο ένα value
{
  int counter = 0;
  char *metaptr;
  char *temprecptr;
  datablock datastruct;
  datablockpointer datablockptr;

  metaptr = metadata + sizeof(blocktype);
  memcpy(&datastruct,metaptr,sizeof(datablock));
  temprecptr = open_scans[scanDesc].recs;                                       //Δείκτης στην αρχή του buffer recs
  while(counter < open_scans[scanDesc].maxrecords || datastruct.data1 != NULL)  //Όσο υπάρχουν έγκυρες εγγραφές
  {
    if(open_files[open_scans[scanDesc].fileDesc].attrType1 == INTEGER)          //Ανάλογα με τον τύπο του κλειδιού
    {
      if( (*(int *)datastruct.data1) > (*(int *)open_scans[scanDesc].value))    //Αν η τιμή είναι μεγαλύτερη απο αυτη του value
      {
        memcpy(temprecptr,&datastruct,sizeof(datablock));                       //Εγγραφή της τιμής στο recs
        temprecptr += sizeof(datablock);                                        //Προχωράμε τον temprecptr
        open_scans[scanDesc].reccounter++;                                      //Αυξάνουμε τις εγγραφές γι αυτό το block
      }
    }
    if(open_files[open_scans[scanDesc].fileDesc].attrType1 == FLOAT)
    {
      if( (*(float *)datastruct.data1) > (*(float *)open_scans[scanDesc].value))
      {
        memcpy(temprecptr,&datastruct,sizeof(datablock));
        temprecptr += sizeof(datablock);
        open_scans[scanDesc].reccounter++;
      }
    }
    if(open_files[open_scans[scanDesc].fileDesc].attrType1 == STRING)
    {
      if(strcmp((char *)datastruct.data1 , (char *)open_scans[scanDesc].value) > 0)
      {
        memcpy(temprecptr,&datastruct,sizeof(datablock));
        temprecptr += sizeof(datablock);
        open_scans[scanDesc].reccounter++;
      }
    }
    metaptr += sizeof(datablock);
    memcpy(&datastruct,metaptr,sizeof(datablock));
    counter++;
  }
  memcpy(&datablockptr,metaptr,sizeof(datablockpointer));
  open_scans[scanDesc].next_block = datablockptr.NextDataBlock;                   //Η μεταβλητή next_block παίρνει την τιμή του επόμενου block δεδομένων
  return;
}


void FindEqualValues(int scanDesc, char *metadata)                      //Αποθηκεύει τις εγγραφές με κλειδί ίσο με ένα value
{
  int counter = 0;
  char *metaptr;
  char *temprecptr;
  datablock datastruct;

  metaptr = metadata + sizeof(blocktype);
  memcpy(&datastruct,metaptr,sizeof(datablock));
  temprecptr = open_scans[scanDesc].recs;                                         //Δείκτης στην αρχή του buffer recs
  while(counter < open_scans[scanDesc].maxrecords || datastruct.data1 != NULL)    //Όσο υπάρχουν έγκυρες εγγραφές
  {
    if(open_files[open_scans[scanDesc].fileDesc].attrType1 == INTEGER)            //Ανάλογα με τον τύπο του κλειδιού
    {
      if( (*(int *)datastruct.data1) == (*(int *)open_scans[scanDesc].value))     //Αν η τιμή είναι ίση με αυτη του value
      {
        memcpy(temprecptr,&datastruct,sizeof(datablock));                         //Εγγραφή της τιμής στο recs
        temprecptr += sizeof(datablock);                                          //Προχωράμε τον temprecptr
        open_scans[scanDesc].reccounter++;                                        //Αυξάνουμε τις εγγραφές γι αυτό το block
      }
    }
    if(open_files[open_scans[scanDesc].fileDesc].attrType1 == FLOAT)
    {
      if( (*(float *)datastruct.data1) == (*(float *)open_scans[scanDesc].value))
      {
        memcpy(temprecptr,&datastruct,sizeof(datablock));
        temprecptr += sizeof(datablock);
        open_scans[scanDesc].reccounter++;
      }
    }
    if(open_files[open_scans[scanDesc].fileDesc].attrType1 == STRING)
    {
      if(strcmp((char *)datastruct.data1 , (char *)open_scans[scanDesc].value) == 0)
      {
        memcpy(temprecptr,&datastruct,sizeof(datablock));
        temprecptr += sizeof(datablock);
        open_scans[scanDesc].reccounter++;
      }
    }
    metaptr += sizeof(datablock);
    memcpy(&datastruct,metaptr,sizeof(datablock));
    counter++;
  }
  open_scans[scanDesc].next_block = -1;                                         //Η μεταβλητή next_block παίρνει την τιμή -1 καθώς δεν θέλουμε να ψάξουμε άλλο block
  return;
}

int RecordEquality(datablock db, void* value1, void* value2, int fileDesc)      //Συνάρτηση που ελέγχει αν η δομή datablock περιέχει τις τιμες value1 value2
{
  if(open_files[fileDesc].attrType1 == INTEGER)
  {
    if((*(int *)db.data1) == (*(int *)value1))
    {
      if(open_files[fileDesc].attrType2 == INTEGER)
      {
        if((*(int *)db.data2) == (*(int *)value2))
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
      else if(open_files[fileDesc].attrType1 == FLOAT)
      {
        if((*(float *)db.data2) == (*(float *)value2))
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
      else
      {
        if(strcmp((char *)db.data2, (char *)value2) == 0)
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
    }
    else
    {
      return 0;
    }
  }
  else if(open_files[fileDesc].attrType1 == FLOAT)
  {
    if((*(float *)db.data1) == (*(float *)value1))
    {
      if(open_files[fileDesc].attrType2 == INTEGER)
      {
        if((*(int *)db.data2) == (*(int *)value2))
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
      else if(open_files[fileDesc].attrType1 == FLOAT)
      {
        if((*(float *)db.data2) == (*(float *)value2))
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
      else
      {
        if(strcmp((char *)db.data2 , (char *)value2) == 0)
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
    }
    else
    {
      return 0;
    }
  }
  else
  {
    if(strcpy((char *)db.data1,(char *)value1) == 0)
    {
      if(open_files[fileDesc].attrType2 == INTEGER)
      {
        if((*(int *)db.data2) == (*(int *)value2))
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
      else if(open_files[fileDesc].attrType1 == FLOAT)
      {
        if((*(float *)db.data2) == (*(float *)value2))
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
      else
      {
        if(strcmp((char *)db.data2 , (char *)value2) == 0)
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
    }
    else
    {
      return 0;
    }
  }
}

void AM_Init() {
  BF_Init(MRU);                          // Αρχικοποίηση του επιπέδου μπλοκ
  int counter;

  for(counter=0; counter<MAX_OPEN_FILES; counter++)     // Αρχικοποίηση του πίνακα των ανοιχτών αρχείων
  {
    strcpy(open_files[counter].fileName,"NONE");
    open_files[counter].attrType1 = ' ';
    open_files[counter].attrLength1 = 0;
    open_files[counter].attrType2 = ' ';
    open_files[counter].attrLength2 = 0;
  }
  for(counter=0; counter<MAX_OPEN_SCANS; counter++)     // Αρχικοποίηση του πίνακα των ανοιχτών σαρώσεων
  {
    open_scans[counter].fileDesc = -1;
    open_scans[counter].block_number = -1;
    open_scans[counter].next_block = -1;
    open_scans[counter].op = -1;
    open_scans[counter].maxrecords = (BF_BLOCK_SIZE - sizeof(datablockpointer))/sizeof(datablock);
    open_scans[counter].reccounter = 0;
    open_scans[counter].value = NULL;
    open_scans[counter].recs = NULL;
    open_scans[counter].recsptr = open_scans[counter].recs;
  }
	return;
}


int AM_CreateIndex(char *fileName,
	               char attrType1,
	               int attrLength1,
	               char attrType2,
	               int attrLength2) {

  int fileDesc,rootpos,counter,negone;
  char *metadata, *metaptr;
  blocktype bt;
  indexblock ib;
  BF_Block* block;
  BF_ErrorCode bferror;

  if(!TypeCheck(attrType1,attrLength1))                   //Έλεγχος του attrType1 και attrLength1
  {
    AM_errno = AME_TCE;
    return AM_errno;
  }

  if(!TypeCheck(attrType2,attrLength2))                   //Έλεγχος του attrType2 και attrLength2
  {
    AM_errno = AME_TCE;
    return AM_errno;
  }

  if(bferror = BF_CreateFile(fileName) != BF_OK)          //Δημιουργία αρχείου με όνομα fileName
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  if(bferror = BF_OpenFile(fileName,&fileDesc) != BF_OK)  //Άνοιγμα αρχείου
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  BF_Block_Init(&block);                                  //Αρχικοποίηση της δομής BF_Block

  if(bferror = BF_AllocateBlock(fileDesc,block) != BF_OK) //Δέσμευση νέου block στο αρχείο
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  rootpos = 1;
  metadata = BF_Block_GetData(block);                     //Παίρνουμε δείκτη στα δεδομένα του πρώτου block
  metaptr = metadata;                                     //Βοηθητικός δείκτης στα δεδομένα του πρώτου block
  memcpy(metadata, "B+TREE", sizeof("B+TREE"));           //Προσθήκη των metadata στο πρώτο block
  metaptr += sizeof("B+TREE");
  memcpy(metaptr, fileName, sizeof(char[100]));
  metaptr += sizeof(char[100]);
  memcpy(metaptr, &attrType1, sizeof(char));
  metaptr += sizeof(char);
  memcpy(metaptr, &attrLength1, sizeof(int));
  metaptr += sizeof(int);
  memcpy(metaptr, &attrType2, sizeof(char));
  metaptr += sizeof(char);
  memcpy(metaptr, &attrLength2, sizeof(int));
  metaptr += sizeof(int);
  memcpy(metaptr, &rootpos, sizeof(int));                        //Προσθήκη της θέσης του root στα metadata

  BF_Block_SetDirty(block);

  if(bferror = BF_UnpinBlock(block) != BF_OK)             //Unpin του metadata block
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  if(bferror = BF_AllocateBlock(fileDesc,block) != BF_OK) //Δέσμευση του root block
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  negone = -1;
  bt.DataBlock = 0;
  bt.NumberOfStructs = 0;
  ib.Pointer = -1;
  ib.Key = NULL;
  metadata = BF_Block_GetData(block);
  metaptr = metadata;
  memcpy(metadata,&bt,sizeof(blocktype));                         //Γέμισμα του block ρίζας
  metaptr += sizeof(blocktype);
  for(counter = 0; counter < ((BF_BLOCK_SIZE - sizeof(int) - sizeof(blocktype))/sizeof(indexblock)); counter++) 
  {
    memcpy(metaptr,&ib,sizeof(indexblock));
    metaptr += sizeof(indexblock);
  }
  memcpy(metaptr,&negone,sizeof(int));

  BF_Block_SetDirty(block);

  if(bferror = BF_UnpinBlock(block) != BF_OK)             //Unpin του root block
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  if(bferror = BF_CloseFile(fileDesc) != BF_OK)           //Κλείσιμο του ανοιγμένου αρχείου
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  BF_Block_Destroy(&block);                               //Καταστροφή της δομής block

  AM_errno = AME_OK;
  return AM_errno;
}


int AM_DestroyIndex(char *fileName) {
  
  int counter;
  char rmcommand[50];

  counter = 0;
  while(counter<MAX_OPEN_FILES)                             //Έλεγχος για ενεργά ανοίγματα του συγκεκριμένου αρχείου
  {
    if(strcmp(fileName,open_files[counter].fileName) == 0)  //Αν υπάρχουν επιστροφή λάθους
    {
      AM_errno = AME_FAO;
      return AM_errno;
    }
    counter++;
  }

  sprintf(rmcommand, "rm %s", fileName);                    //Αν δεν υπάρχουν διαγραφή του αρχείου απο τον δίσκο
  system(rmcommand);
  
  AM_errno = AME_OK;
  return AME_OK;
}


int AM_OpenIndex (char *fileName) {
  
  BF_ErrorCode bferror;
  int fileDesc;
  int counter,pos;
  char *metadata, *metadataptr;
  BF_Block *block;

  if(bferror = BF_OpenFile(fileName,&fileDesc) != BF_OK)                 //Άνοιγμα αρχείου
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  counter = 0;
  while(counter < MAX_OPEN_FILES)                                       //Αναζήτηση στον πίνακα των ανοιχτών αρχείων για κενό κελί
  {
    if(strcmp(open_files[counter].fileName,"NONE") == 0)                //Εύρεση κενού κελιού
    {
      BF_Block_Init(&block);                                            //Αρχικοποίηση δομής block
      if(bferror = BF_GetBlock(fileDesc,0,block) != BF_OK)              //Απόκτηση του block με τα metadata του αρχείου
      {
        BF_PrintError(bferror);
        AM_errno = AME_BFE;
        return AM_errno;
      }
      metadata = BF_Block_GetData(block);                               //Εγγραφή των metadata του αρχείου στην θέση του στον πίνακα open_files
      metadataptr = metadata + sizeof("B+TREE");
      memcpy(open_files[counter].fileName, metadataptr, sizeof(char[100]));  
      metadataptr += sizeof(char[100]);
      memcpy(&open_files[counter].attrType1, metadataptr, sizeof(char));
      metadataptr += sizeof(char);
      memcpy(&open_files[counter].attrLength1, metadataptr, sizeof(int));
      metadataptr += sizeof(int);
      memcpy(&open_files[counter].attrType2, metadataptr, sizeof(char));
      metadataptr += sizeof(char);
      memcpy(&open_files[counter].attrLength2, metadataptr, sizeof(int));
      
      pos = counter;
      counter = MAX_OPEN_FILES;
    }
    counter++;
  }

  if(bferror = BF_UnpinBlock(block) != BF_OK)                          //Ξεκαρφίτσωμα του block που πήραμε
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  BF_Block_Destroy(&block);

  if(bferror = BF_CloseFile(fileDesc) != BF_OK)
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  if(counter > MAX_OPEN_FILES)                                           //Έχει βρεθεί κενή θέση στον πίνακα
  {
    return pos;
  }
  else                                                                   //Δεν έχει βρεθεί κενή θέση στον πίνακα
  {
    AM_errno = AME_NSO;
    return AM_errno;
  }
}


int AM_CloseIndex (int fileDesc) {

  int counter;

  counter = 0;
  while(counter < MAX_OPEN_SCANS)                                   //Έλεγχος για ανοιχτές σαρώσεις του αρχείου με αριθμό fileDesc
  {
    if(open_scans[counter].fileDesc == fileDesc)                    //Αν υπάρχουν ανοιχτές σαρώσεις επιστρέφεται κωδικός λάθους
    {
      AM_errno = AME_FBS;
      return AM_errno;
    }
    counter++;
  }

  strcpy(open_files[fileDesc].fileName, "NONE");                    //Εξαγωγή του αρχείου απο τον πίνακα ανοιχτών αρχείων 
  open_files[fileDesc].attrType1 = ' ';
  open_files[fileDesc].attrLength1 = 0;
  open_files[fileDesc].attrType2 = ' ';
  open_files[fileDesc].attrLength2 = 0;

  AM_errno = AME_OK;
  return AM_errno;
}

int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
  
  BF_ErrorCode bferror;
  BF_Block *block, *new_block,*meta_block;
  indexblock ib;
  blocktype bt,tempbt;
  datablock db;
  stack insert_stack;
  datablock *dbarray;
  indexblock *ibarray;
  datablockpointer datablock_nextpointer, tempdbp, dbp;
  char *metadata, *metaptr;
  int tempptr;
  int half_array,loop_end,root,new_num,ibhalf_array;
  int fd,target_block,counter,key_pos,number_of_blocks,newblock_id;
  void *index_key;

  if(bferror = BF_OpenFile(open_files[fileDesc].fileName,&fd) != BF_OK)  
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  BF_Block_Init(&block);

  if(bferror = BF_GetBlock(fd,0,block) != BF_OK)
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  metadata = BF_Block_GetData(block);
  metaptr = metadata + sizeof("B+TREE") + sizeof(char[100]) + sizeof(char) + sizeof(int) + sizeof(char) + sizeof(int);
  memcpy(&root,metaptr,sizeof(int));

  if(bferror = BF_UnpinBlock(block) != BF_OK)
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  target_block = SearchData(root,value1,fd,fileDesc,&insert_stack);     //Εύρεση του block που περιέχει την τιμή value1

   if(target_block == -1)                                             //Αν στο δέντρο υπάρχει μόνο η ρίζα
  {
    if(bferror = BF_GetBlock(fd,root,block) != BF_OK)                //Προσπέλαση του block ρίζας
    {
      BF_PrintError(bferror);
      AM_errno = AME_BFE;
      return AM_errno;
    } 

    BF_Block_Init(&new_block);

    if(bferror = BF_AllocateBlock(fd,new_block) != BF_OK)            //Δέσμευση του πρώτου data block
    {
      BF_PrintError(bferror);
      AM_errno = AME_BFE;
      return AM_errno;
    }
    
    if(bferror = BF_GetBlockCounter(fd,&number_of_blocks) != BF_OK)
    {
      BF_PrintError(bferror);
      AM_errno = AME_BFE;
      return AM_errno;
    }

    metadata = BF_Block_GetData(block);
    memcpy(&bt,metadata,sizeof(blocktype));
    bt.NumberOfStructs++;
    memcpy(metadata,&bt,sizeof(blocktype));
    metaptr = metadata + sizeof(blocktype);
    ib.Pointer = number_of_blocks - 1;                               //Ο pointer δείχνει στο block που μόλις δεσεύτηκε
    ib.Key = value1;
    memcpy(metaptr,&ib,sizeof(indexblock));

    BF_Block_SetDirty(block);

    if(bferror = BF_UnpinBlock(block) != BF_OK)
    {
      BF_PrintError(bferror);
      AM_errno = AME_BFE;
      return AM_errno;
    }

    bt.DataBlock = 1;
    bt.NumberOfStructs = 1;
    db.data1 = value1;
    db.data2 = value2;
    dbp.NextDataBlock = -1; 
    metadata = BF_Block_GetData(new_block);                         //Αρχικοποίηση του πρώτου data block
    memcpy(metadata,&bt,sizeof(blocktype));
    metaptr = metadata + sizeof(blocktype);
    memcpy(metaptr,&db,sizeof(datablock));
    metaptr += sizeof(datablock);
    db.data1 = NULL;
    db.data2 = NULL;
    for(counter = 0; counter < ((BF_BLOCK_SIZE - sizeof(blocktype) - sizeof(datablock) - sizeof(datablockpointer))/sizeof(datablock)); counter++)
    {
      memcpy(metaptr,&db,sizeof(datablock));
      metaptr += sizeof(datablock);
    }
    memcpy(metaptr,&dbp,sizeof(datablockpointer));

    BF_Block_SetDirty(new_block);

    if(bferror = BF_UnpinBlock(new_block) != BF_OK)
    {
      BF_PrintError(bferror);
      AM_errno = AME_BFE;
      return AM_errno;
    }
  
    BF_Block_Destroy(&block);
    BF_Block_Destroy(&new_block);
  }
  else                                                            //Αν υπάρχουν block δεδομένων στο αρχείο
  {
    if(bferror = BF_GetBlock(fd,target_block,block) != BF_OK)
    {
      BF_PrintError(bferror);
      AM_errno = AME_BFE;
      return AM_errno;
    }

    metadata = BF_Block_GetData(block);
    metaptr = metadata;
    metaptr += sizeof(blocktype);
    memcpy(&bt,metadata,sizeof(blocktype));
    for(counter = 0; counter < bt.NumberOfStructs; counter++)   //Έλεγχος αν το ζευγάρι value1,value2 υπάρχει ήδη στο αρχείο
    {
      memcpy(&db,metaptr,sizeof(datablock));
      if(RecordEquality(db,value1,value2,fileDesc))
      {
        AM_errno = AME_RAE;
        return AM_errno;
      }
      metaptr += sizeof(blocktype);
    }

    if(bferror = BF_UnpinBlock(block) != BF_OK)
    {
      BF_PrintError(bferror);
      AM_errno = AME_BFE;
      return AM_errno;
    }

    if(bferror = BF_GetBlock(fd,target_block,block) != BF_OK)                //Προσπέλαση του block που περιέχει την τιμή value1
    {
      BF_PrintError(bferror);
      AM_errno = AME_BFE;
      return AM_errno;
    }

    metadata = BF_Block_GetData(block);
    metaptr = metadata + sizeof(blocktype);
    memcpy(&bt,metadata,sizeof(blocktype));

    if(bt.NumberOfStructs < (BF_BLOCK_SIZE - sizeof(blocktype) - sizeof(datablockpointer))/sizeof(datablock)) //Αν υπάρχει χώρος στο block δεδομένων εισαγωγή
    {
      metaptr += bt.NumberOfStructs * sizeof(datablock);
      db.data1 = value1;
      db.data2 = value2;
      bt.NumberOfStructs++;
      memcpy(metaptr, &db, sizeof(datablock));
      memcpy(metadata, &bt, sizeof(blocktype));
      
      BF_Block_SetDirty(block);
    }
    else                                                                                                      //Αν δεν υπάρχει χώρος σπάμε το block
    {
      dbarray = (datablock*) malloc((bt.NumberOfStructs + 1) * sizeof(datablock));      //Δεσμεύουμε χώρο για Number0fStructs + 1 δομές
      for(counter = 0; counter < bt.NumberOfStructs; counter++)                         //Αντιγραφή του block δεδομένων στον πίνακα
      {
        memcpy(&db,metaptr,sizeof(datablock));
        dbarray[counter].data1 = db.data1;
        dbarray[counter].data2 = db.data2;
        metaptr += sizeof(datablock);
      }
      counter = 0;
      key_pos = -1;
      while(counter < bt.NumberOfStructs)                                               //Εύρεση θέσης εισαγωγής του ζευγαριού value1,value2
      {
        if(Comparison(fileDesc,value1,dbarray[counter].data1))
        {
          key_pos = counter;
        }
        counter++;
      }
      if(key_pos == -1)
      {
        key_pos = counter + 1;
      }

      counter = bt.NumberOfStructs + 1;                                                 //Μεταφορά των στοιχείων του πίνακα στις κατάλληλες θέσεις και εισαγωγή του καινούριου
      while(counter > key_pos)
      {
        dbarray[counter].data1 = dbarray[counter - 1].data1;
        dbarray[counter].data2 = dbarray[counter - 1].data2;
      }
      dbarray[key_pos].data1 = value1;
      dbarray[key_pos].data2 = value2;
    
      half_array = ceil((float)((bt.NumberOfStructs + 1) / 2));

      if(bferror = BF_AllocateBlock(fd,new_block) != BF_OK)                             //Δέσμευση καινούριου block για το σπάσιμο
      {
        BF_PrintError(bferror);
        AM_errno = AME_BFE;
        return AM_errno;
      }

      if(bferror = BF_GetBlockCounter(fd,&newblock_id) != BF_OK)
      {
        BF_PrintError(bferror);
        AM_errno = AME_BFE;
        return AM_errno;
      }
      newblock_id -= 1;

      metaptr = metadata + sizeof(blocktype);                                           //Στο πρώτο block μένουν τα πρώτα half_array στοιχεία του πίνακα.
      for(counter = 0; counter < bt.NumberOfStructs + 1; counter++)
      {
        if(counter >= half_array)
        {
          db.data1 = NULL;
          db.data2 = NULL;
          memcpy(metaptr,&db,sizeof(datablock));
        }
        else
        {
          db.data1 = dbarray[counter].data1;
          db.data2 = dbarray[counter].data2;
          memcpy(metaptr,&db,sizeof(datablock));
        }
        metaptr += sizeof(datablock);
      }
      tempbt.NumberOfStructs = half_array;                                              //Αλλάζουμε το πλήθος των δομών στο block σε half_array
      tempbt.DataBlock = 1;
      memcpy(metadata,&tempbt,sizeof(blocktype));
      memcpy(&datablock_nextpointer,metaptr,sizeof(datablockpointer));
      tempdbp.NextDataBlock = newblock_id;
      memcpy(metaptr,&tempdbp,sizeof(datablockpointer));                                     //Ο δείκτης του πρώτου block δείχνει στο καινούριο block
      BF_Block_SetDirty(block);
    
      tempbt.NumberOfStructs = bt.NumberOfStructs - half_array + 1;                     //Το νέο block έχει πλήθος δομών NumberOfStructs + 1 - half_array
      tempbt.DataBlock = 1;
      metadata = BF_Block_GetData(new_block);
      metaptr = metadata + sizeof(blocktype);
      memcpy(metadata,&tempbt,sizeof(blocktype));
      for(counter = 0; counter < bt.NumberOfStructs + 1; counter++)                     //Στο καινούριο block μένουν τα υπόλοιπα στοιχεία του πίνακα
      {
        if(counter >= bt.NumberOfStructs - half_array + 1)
        {
          db.data1 = NULL;
          db.data2 = NULL;
          memcpy(metaptr,&db,sizeof(datablock));
        }
        else
        {
          db.data1 = dbarray[counter + half_array].data1;
          db.data2 = dbarray[counter + half_array].data2;
          memcpy(metaptr,&db,sizeof(datablock));
        }
        metaptr += sizeof(datablock);
      }
      memcpy(metaptr,&datablock_nextpointer,sizeof(datablockpointer));                               //Το νέο block δείχνει εκεί που έδειχνε πριν το block
      BF_Block_SetDirty(new_block);

      if(bferror = BF_UnpinBlock(block) != BF_OK)
      {
        BF_PrintError(bferror);
        AM_errno = AME_BFE;
        return AM_errno;
      }

      if(bferror = BF_UnpinBlock(block) != BF_OK)
      {
        BF_PrintError(bferror);
        AM_errno = AME_BFE;
        return AM_errno;
      }
      index_key = dbarray[half_array].data1;
      free(dbarray);
      
      loop_end = 0;                                                                     //Εισαγωγή κατάλληλων τιμών στα block ευρετηρίου
      while(!loop_end)
      {
        if(EmptyStack(&insert_stack))                                                   //Εαν η στοίβα είναι άδεια πρέπει να σπάσει η ρίζα
        {
          if(bferror = BF_AllocateBlock(fd,block))
          {
            BF_PrintError(bferror);
            AM_errno = AME_BFE;
            return AM_errno;
          }

          if(bferror = BF_GetBlockCounter(fd,&number_of_blocks))
          {
            BF_PrintError(bferror);
            AM_errno = AME_BFE;
            return AM_errno;
          }
          number_of_blocks -= 1;

          if(bferror = BF_GetBlock(fd,0,meta_block))
          {
            BF_PrintError(bferror);
            AM_errno AME_BFE;
            return AM_errno;
          }

          metadata = BF_Block_GetData(meta_block);
          metaptr = metadata + sizeof("B+TREE") + sizeof(char[100]) +  sizeof(char) + sizeof(int) + sizeof(char) + sizeof(int);
          memcpy(metaptr,&number_of_blocks,sizeof(int));                                 //Αλλάζουμε τον δείκτη στη ρίζα στα metadata
          BF_Block_SetDirty(meta_block);

          if(bferror = BF_UnpinBlock(meta_block))
          {
            BF_PrintError(bferror);
            AM_errno = AME_BFE;
            return AM_errno;
          }
        
          bt.DataBlock = 0;
          bt.NumberOfStructs = 1;
          ib.Pointer = target_block;
          ib.Key = index_key;
          metadata = BF_Block_GetData(block);
          metaptr = metadata;
          memcpy(metadata,&bt,sizeof(blocktype));                         //Γέμισμα του block ρίζας
          metaptr += sizeof(blocktype);
          memcpy(metaptr,&ib,sizeof(indexblock));
          metaptr += sizeof(indexblock);
          ib.Pointer = -1;
          ib.Key = NULL;
          for(counter = 1; counter < ((BF_BLOCK_SIZE - sizeof(int) - sizeof(blocktype))/sizeof(indexblock)); counter++) 
          {
            memcpy(metaptr,&ib,sizeof(indexblock));
            metaptr += sizeof(indexblock);
          }
          memcpy(metaptr,&newblock_id,sizeof(int));
          loop_end = 1;
        }
        else                                                              //Η στοίβα δεν είναι άδεια
        {
          Pop(&insert_stack,&target_block);
          
          if(bferror = BF_GetBlock(fd,target_block,block) != BF_OK)
          {
            BF_PrintError(bferror);
            AM_errno = AME_BFE;
            return AM_errno;
          }

          metadata = BF_Block_GetData(block);
          metaptr = metadata + sizeof(blocktype);
          memcpy(&bt,metadata,sizeof(blocktype));

          if(bt.NumberOfStructs < ((BF_BLOCK_SIZE - sizeof(blocktype) - sizeof(int))/sizeof(indexblock)))   //Υπάρχει χώρος για εισαγωγή στο block
          {
            ibarray = (indexblock*) malloc((bt.NumberOfStructs + 1) * sizeof(indexblock));
            for(counter = 0; counter < bt.NumberOfStructs; counter++)
            {
              memcpy(&ib,metaptr,sizeof(indexblock));
              ibarray[counter].Pointer = ib.Pointer;
              ibarray[counter].Key = ib.Key;
              metaptr += sizeof(indexblock);
            }

            counter = 0;
            key_pos = -1;
            while(counter < bt.NumberOfStructs)
            {
              if(Comparison(fileDesc,index_key,ibarray[counter].Key))
              {
                key_pos = counter;
              }
              counter++;
            }
            
            if(key_pos == -1)
            {
              metaptr = metadata + BF_BLOCK_SIZE - sizeof(int);
              memcpy(metaptr,&newblock_id,sizeof(int));
            }
            else
            {
              counter = bt.NumberOfStructs + 1;                                                 //Μεταφορά των στοιχείων του πίνακα στις κατάλληλες θέσεις και εισαγωγή του καινούριου
              while(counter > key_pos)
              {
                ibarray[counter].Pointer = ibarray[counter - 1].Pointer;
                ibarray[counter].Key = ibarray[counter - 1].Key;
              }
              ibarray[key_pos].Pointer = newblock_id;
              ibarray[key_pos].Key = index_key;
            
              bt.NumberOfStructs += 1;
              metaptr = metadata;
              memcpy(metaptr,&bt,sizeof(blocktype));
              metaptr += sizeof(blocktype);
              for(counter = 0; counter < bt.NumberOfStructs; counter++)
              {
                memcpy(metaptr,&ibarray[counter],sizeof(indexblock));
                metaptr += sizeof(indexblock);
              }
            }
            BF_Block_SetDirty(block);
            free(ibarray);

            if(bferror = BF_UnpinBlock(block) != BF_OK)
            {
              BF_PrintError(bferror);
              AM_errno = AME_BFE;
              return AM_errno;
            }

            loop_end = 1;
          }
          else                                                                                //Εαν δεν υπάρχει χώρος για εισαγωγή στο block
          {
            ibarray = (indexblock*) malloc((bt.NumberOfStructs + 2) * sizeof(indexblock));      //Δεσμεύουμε χώρο για Number0fStructs + 1 δομές
            for(counter = 0; counter < bt.NumberOfStructs; counter++)                         //Αντιγραφή του block δεδομένων στον πίνακα
            {
              memcpy(&ib,metaptr,sizeof(indexblock));
              ibarray[counter].Pointer = ib.Pointer;
              ibarray[counter].Key = ib.Key;
              metaptr += sizeof(indexblock);
            }
            metaptr = metadata + BF_BLOCK_SIZE - sizeof(int);
            memcpy(&tempptr,metaptr,sizeof(int));
            ibarray[counter].Pointer = tempptr;
            ibarray[counter].Key = NULL;

            counter = 0;
            key_pos = -1;
            while(counter < bt.NumberOfStructs)
            {
              if(ibarray[counter].Key != NULL)
              {
                if(Comparison(fileDesc,index_key,ibarray[counter].Key))
                {
                  key_pos = counter;
                }
              }
              counter++;
            }
            if(key_pos == -1)
            {
              key_pos = counter + 1;
            }

            counter = bt.NumberOfStructs + 2;                                                 //Μεταφορά των στοιχείων του πίνακα στις κατάλληλες θέσεις και εισαγωγή του καινούριου
            while(counter > key_pos)
            {
              ibarray[counter].Pointer = ibarray[counter - 1].Pointer;
              ibarray[counter].Key = ibarray[counter - 1].Key;
            }
            ibarray[key_pos].Pointer = newblock_id;
            ibarray[key_pos].Key = index_key;
    
            ibhalf_array = floor((float)((bt.NumberOfStructs + 2) / 2));

            if(bferror = BF_AllocateBlock(fd,new_block) != BF_OK)                             //Δέσμευση καινούριου block για το σπάσιμο
            {
              BF_PrintError(bferror);
              AM_errno = AME_BFE;
              return AM_errno;
            }
            if(bferror = BF_GetBlockCounter(fd,&newblock_id) != BF_OK)
            {
              BF_PrintError(bferror);
              AM_errno = AME_BFE;
              return AM_errno;
            }
            newblock_id -= 1;

            metaptr = metadata + sizeof(blocktype);                                           //Στο πρώτο block μένουν τα πρώτα ibhalf_array στοιχεία του πίνακα.
            for(counter = 0; counter < bt.NumberOfStructs + 2; counter++)
            {
              if(counter >= ibhalf_array - 1)
              {
                ib.Pointer = -1;
                ib.Key = NULL;
                memcpy(metaptr,&ib,sizeof(indexblock));
              }
              else
              {
                ib.Pointer = ibarray[counter].Pointer;
                ib.Key = ibarray[counter].Key;
                memcpy(metaptr,&ib,sizeof(indexblock));
              }
              metaptr += sizeof(indexblock);
            }
            tempbt.NumberOfStructs = ibhalf_array;                                              //Αλλάζουμε το πλήθος των δομών στο block σε ibhalf_array
            tempbt.DataBlock = 0;
            memcpy(metadata,&tempbt,sizeof(blocktype));
            BF_Block_SetDirty(block);
    
            tempbt.NumberOfStructs = bt.NumberOfStructs - half_array + 2;                     //Το νέο block έχει πλήθος δομών NumberOfStructs + 1 - half_array
            tempbt.DataBlock = 0;
            metadata = BF_Block_GetData(new_block);
            metaptr = metadata + sizeof(blocktype);
            memcpy(metadata,&tempbt,sizeof(blocktype));
            for(counter = 0; counter < bt.NumberOfStructs + 2; counter++)                     //Στο καινούριο block μένουν τα υπόλοιπα στοιχεία του πίνακα
            {
              if(counter >= bt.NumberOfStructs - half_array + 2)
              {
                ib.Pointer = -1;
                ib.Key = NULL;
                memcpy(metaptr,&ib,sizeof(indexblock));
              }
              else
              {
                ib.Pointer = ibarray[counter + ibhalf_array].Pointer;
                ib.Key = ibarray[counter + ibhalf_array].Key;
                memcpy(metaptr,&ib,sizeof(indexblock));
              }
              metaptr += sizeof(indexblock);
            }
            BF_Block_SetDirty(new_block);

            if(bferror = BF_UnpinBlock(block) != BF_OK)
            {
              BF_PrintError(bferror);
              AM_errno = AME_BFE;
              return AM_errno;
            }

            if(bferror = BF_UnpinBlock(block) != BF_OK)
            {
              BF_PrintError(bferror);
              AM_errno = AME_BFE;
              return AM_errno;
            }
            index_key = ibarray[ibhalf_array].Key;
            free(ibarray);
          }
        }
      }
    }
  }
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&new_block);
  
  if(bferror = BF_CloseFile(fd) != BF_OK)
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  return AME_OK;
}


int AM_OpenIndexScan(int fileDesc, int op, void *value) {
  
  BF_ErrorCode bferror;
  BF_Block *block;
  char *metadata, *metaptr;
  int counter = 0;
  int pos,fd,root;

  if(strcmp(open_files[fileDesc].fileName, "NONE") == 0)            //Έλεγχος αν το αρχείο έχει όντως ανοιχθεί
  {
    AM_errno = AME_FNO;
    return AM_errno;
  }

  while(counter < MAX_OPEN_SCANS)                                   //Έλεγχος αν υπάρχει κενη θέση στον πίνακα σαρώσεων
  {
    if(open_scans[counter].fileDesc == -1)
    {
      open_scans[counter].fileDesc = fileDesc;
      pos = counter;
      counter = MAX_OPEN_SCANS;
    }
    counter++;
  }

  if(counter == MAX_OPEN_SCANS)                                      //Αν δεν βρεθεί κενή θέση
  {
    AM_errno = AME_NSS;
    return AM_errno;
  }

  if(bferror = BF_OpenFile(open_files[fileDesc].fileName,&fd) != BF_OK)   //Άνοιγμα αρχείου απο blocks
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  BF_Block_Init(&block);                                            //Αρχικοποίηση της δομής block

  if(bferror = BF_GetBlock(fd,0,block) != BF_OK)                    //Προσπέλαση του block με τα metadata του αρχείου
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  metadata = BF_Block_GetData(block);
  metaptr = metadata;
  metaptr += sizeof("B+TREE") + sizeof(char[100]) + sizeof(char) + sizeof(int) + sizeof(char) + sizeof(int);
  memcpy(&root,metaptr,sizeof(int));                                //Εύρεση του αριθμού του block ρίζας

  if(bferror = BF_UnpinBlock(block) != BF_OK)
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  open_scans[pos].block_number = SearchData(root,value,fd,fileDesc,NULL);             //Αναζήτηση για το πρώτο block δεδομένων που περιέχει το value 
  open_scans[pos].op = op;
  open_scans[pos].value = value;

  if(op == EQUAL || op == GREATER_THAN || op == GREATER_THAN_OR_EQUAL || op == NOT_EQUAL)
  {
    open_scans[pos].next_block = open_scans[pos].block_number;      //Εκχώρηση του block που περιέχε το value στην τιμή next_block
  }
  else
  {
    open_scans[pos].next_block = FindFirstDataBlock(root,fd,fileDesc);      //Στην τιμή next_block εκχωρείται το πρώτο data block του αρχείου
  }

  if(bferror = BF_CloseFile(fd) != BF_OK)
  {
    BF_PrintError(bferror);
    AM_errno = AME_BFE;
    return AM_errno;
  }

  if(counter > MAX_OPEN_SCANS)                                      //Αν βρεθεί κενή θέση
  {
    return pos;
  }
}


void *AM_FindNextEntry(int scanDesc) {
	
    int fd,helpingblock,operatorflag;
    BF_ErrorCode bferror;
    BF_Block *block;
    datablock returndata;
    char *metadata;

    if(scanDesc < 0 || scanDesc > 20)                             //Σφάλμα στη μεταβλητή scanDesc
    {
      AM_errno = AME_BND;
      return NULL;
    }

    if(open_scans[scanDesc].fileDesc == -1)                       //Το αρχείο το οποίο θέλουμε να σαρώσουμε δεν έχει ανοιχτεί
    {
      AM_errno = AME_FNS;
      return NULL;
    }

    if(open_scans[scanDesc].recs == NULL)                           //Εαν δεν υπάρχουν εγγραφές στο recs προσθήκη των εγγραφών του next_block που ικανοποιούν τον τελεστή op
    {

      if(bferror = BF_OpenFile(open_files[open_scans[scanDesc].fileDesc].fileName,&fd) != BF_OK)  //Άνοιγμα αρχείου
      {
        BF_PrintError(bferror);
        AM_errno = AME_BFE;
        return NULL;
      }
    
      BF_Block_Init(&block);

      if(bferror = BF_GetBlock(fd,open_scans[scanDesc].next_block,block) != BF_OK)                //Προσπέλαση block απο το οποία ξεκινάει η σάρωση
      {
        BF_PrintError(bferror);
        AM_errno = AME_BFE;
        return NULL;
      }

      open_scans[scanDesc].recs = (char *)malloc(open_scans[scanDesc].maxrecords * sizeof(datablock));
      open_scans[scanDesc].recsptr = open_scans[scanDesc].recs;
      metadata = BF_Block_GetData(block);
      helpingblock = open_scans[scanDesc].next_block;
      operatorflag = 0;
      if(open_scans[scanDesc].op == EQUAL || open_scans[scanDesc].op == GREATER_THAN_OR_EQUAL || open_scans[scanDesc].op == LESS_THAN_OR_EQUAL)    
      {
        if(open_scans[scanDesc].op == GREATER_THAN_OR_EQUAL && operatorflag == 0)                   //Εύρεση τιμών ίσων με το value στο τρέχων block για operator >=
        {  
          FindEqualValues(scanDesc,metadata);                                                       
          operatorflag = 1;
        }
        if(open_scans[scanDesc].op == LESS_THAN_OR_EQUAL && open_scans[scanDesc].next_block == -1)
        {
          open_scans[scanDesc].next_block = open_scans[scanDesc].block_number;                      //Όρισμός του block που περιέχει το value ως τρέχωντως block
          FindEqualValues(scanDesc,metadata);                                                       //Εύρεση τιμώμ ίσων με το value στο τρέχων block για operator <=
        }
        if(open_scans[scanDesc].op == EQUAL)
        {
          FindEqualValues(scanDesc,metadata);                                                       //Εύρεση τιμώμ ίσων με το value στο τρέχων block
        }
      }
      if(open_scans[scanDesc].op == GREATER_THAN)
      {
        FindGreaterValues(scanDesc,metadata);                                                     //Εύρεση τιμών μεγαλύτερων του value στο τρέχων block
      }
      if(open_scans[scanDesc].op == LESS_THAN)
      {
        FindLessValues(scanDesc,metadata);                                                        //Εύρεση τιμών μικρότερων του value στο τρέχων block
      }
      if(open_scans[scanDesc].op == GREATER_THAN_OR_EQUAL)
      {
        if(open_scans[scanDesc].next_block == -1)  
        {
          open_scans[scanDesc].next_block = helpingblock;                                           //Όρισμός του block που περιέχει το value ως τρέχωντως block
        }
        FindGreaterValues(scanDesc,metadata);                                                     //Εύρεση τιμών μεγαλύτερων του value στο τρέχων block        
      }
      if(open_scans[scanDesc].op == LESS_THAN_OR_EQUAL)
      {
        FindLessValues(scanDesc,metadata);                                                        //Εύρεση τιμών μικρότερων του value στο τρέχων block
      }
      if(open_scans[scanDesc].op == NOT_EQUAL)
      {
        if(operatorflag == 0)
        {
          FindLessValues(scanDesc,metadata);                                                        //Εύρεση τιμών μικρότερων του value στο τρέχων block
        }
        if(open_scans[scanDesc].next_block == -1)
        {
          open_scans[scanDesc].next_block = open_scans[scanDesc].block_number;                      //Όρισμός του block που περιέχει το value ως τρέχωντως block
          operatorflag = 1;
        }
        if(operatorflag == 1)
        {
          FindGreaterValues(scanDesc,metadata);                                                     //Εύρεση τιμών μεγαλύτερων του value στο τρέχων bloc
        }
      }
      
      if(bferror = BF_UnpinBlock(block) != BF_OK)
      {
        BF_PrintError(bferror);
        AM_errno = AME_BFE;
        return NULL;
      }

      if(bferror = BF_CloseFile(fd) != BF_OK)
      {
        BF_PrintError(bferror);
        AM_errno = AME_BFE;
        return NULL;
      }
    }
    if(open_scans[scanDesc].recs != NULL)                                                         //Εαν υπάρχουν εγγραφές στο recs επιλογή της επόμενης εγγραφής
    {
      if(open_scans[scanDesc].reccounter > 0)
      {
        memcpy(&returndata,open_scans[scanDesc].recsptr,sizeof(datablock));
        open_scans[scanDesc].recsptr += sizeof(datablock);
        open_scans[scanDesc].reccounter--;
        return returndata.data2;
      }
      if(open_scans[scanDesc].reccounter == 0)
      {
        free(open_scans[scanDesc].recs);
        open_scans[scanDesc].recs = NULL;
        open_scans[scanDesc].recsptr = open_scans[scanDesc].recs;
      }
    
    if(open_scans[scanDesc].op == LESS_THAN || open_scans[scanDesc].op == LESS_THAN_OR_EQUAL) //Συνθήκη τερματισμού αρχείου για τους τελεστές: < , <=
    {
      if(open_scans[scanDesc].next_block == open_scans[scanDesc].block_number)
      {
        AM_errno = AME_EOF;
        return NULL;
      }
    }
    else
    {
      if(open_scans[scanDesc].next_block == -1)                     //Συνθήκη τερματισμού αρχείου για τους τελεστές: == , > , >=, !=
      {
        AM_errno = AME_EOF;
        return NULL;
      }
    }
    }
  BF_Block_Destroy(&block);
}


int AM_CloseIndexScan(int scanDesc) {
  
  open_scans[scanDesc].fileDesc = -1;
  open_scans[scanDesc].block_number = -1;
  open_scans[scanDesc].next_block = -1;
  open_scans[scanDesc].op = -1;
  open_scans[scanDesc].reccounter = 0;
  open_scans[scanDesc].value = NULL;
  open_scans[scanDesc].recs = NULL;
  open_scans[scanDesc].recsptr = NULL;

  AM_errno = AME_OK;
  return AME_OK;
}


void AM_PrintError(char *errString) {
  
  printf("%s, ",errString);

  switch(AM_errno)
  {
    case -1:
      printf("End of file reached.\n");
      break;
    case -2:
      printf("Error in block level.\n");
      break;
    case -3:
      printf("Can't delete file because it is still open.\n");
      break;
    case -4:
      printf("No space in open_files array.\n");
      break;
    case -5:
      printf("Can't close file due to open scans on it.\n");
      break;
    case -6:
      printf("The file has not been opened.\n");
      break;
    case -7:
      printf("No space in open_scans array.\n");
      break;
    case -8:
      printf("Type and size does not match.\n");
      break;
    case -9:
      printf("The file has not been opened for scan.\n");
      break;
    case -10:
      printf("Index out of bounds.\n");
      break;
    case -11:
      printf("Record can not be inserted as it already exists.\n");
      break;
  }
}

void AM_Close() {
  
}
