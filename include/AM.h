#ifndef AM_H_
#define AM_H_

/* Error codes */

extern int AM_errno;

#define AME_OK 0
#define AME_EOF -1
#define AME_BFE -2  //Block File Error: Σφάλμα στο επίπεδο block
#define AME_FAO -3  //File Already Opened: Αδυναμία διαγραφής αρχείου λόγω ενεργών ανοιγμάτων
#define AME_NSO -4  //No Space to Open: ΈΛλειψη χώρου γιά άνοιγμα αρχείου
#define AME_FBS -5  //File Being Scanned: Αδυναμία κλεισίματος αρχείου λόγω ανοιχτών σαρώσεων
#define AME_FNO -6  //File Not Opened: Το αρχείο δεν έχει ανοιχτεί
#define AME_NSS -7  //No Space for Scan: Έλλειψη χώρου για άνοιγμα σάρωσης
#define AME_TCE -8  //Type Check Error: Σφάλμα στον έλεγχο τύπων
#define AME_FNS -9  //File Not Scanned: Το αρχείο δεν έχει ανοιχτεί για σάρωση
#define AME_BND -10 //Boundries Error: Δείκτης πίνακα εκτώς ορίων
#define AME_RAE -11 //Recod Already Exists: Η εγγραφή δεν μπορεί να ισαχθεί στο αρχείο γιατι υπάρχει ήδη

#define EQUAL 1
#define NOT_EQUAL 2
#define LESS_THAN 3
#define GREATER_THAN 4
#define LESS_THAN_OR_EQUAL 5
#define GREATER_THAN_OR_EQUAL 6

#define MAX_OPEN_FILES 20       //Μέγιστο πλήθος ανοιχτών αρχείων
#define MAX_OPEN_SCANS 20       // Μεγιστο πλήθος ανοιχτών σαρώσεων

typedef struct File{            //Δομή αρχείου
  char fileName[100];           //Όνομα του αρχείου
  char attrType1;               //Τύπος πρώτου πεδίου
  int attrLength1;              //Μέγεθος πρώτου πεδίου
  char attrType2;               //Τύπος δεύτερου πεδίου
  int attrLength2;              //Μέγεθος δεύτερου πεδίο
} file;

typedef struct Scan{            //Δομή σάρωσης
  int fileDesc;
  int block_number;             //Τρέχον μπλοκ
  int next_block;               //Επόμενο block για αναζήτηση
  int op;                       //Τελεστής σύγκρισης
  int maxrecords;               //Το μέγιστο πλήθος εγγραφών που χωράνε σε ένα data block
  int reccounter;               //Μετριτής τιμών μέσα στο recs 
  void *value;                  //Η τιμή σύγκρισης
  char *recs;                   //Τα records που ικανοποιούν τη συνθήκη σάρωσης σε κάθε block 
  char *recsptr;                //Δείκτης στα recs
} scan;

typedef struct BlockType{       //Δομή τύπου block (block δεδομένων, block ευρετηρίου)
  char DataBlock;               //0 Αν το block είναι index block, 1 αν το block είναι data block
  int NumberOfStructs;          //Πλήθος δομών μέσα στο block
} blocktype;

typedef struct DataBlockPointer{       //Δομή για το block δεδομένων που περιέχει δείκτη στο επόμενο data block
  int NextDataBlock;
} datablockpointer;

typedef struct DataBlock{             //Δομή για το block δεδομένων που περιέχει τα δεδομένα του block
  void *data1;
  void *data2;
} datablock;

typedef struct IndexBlock{      //Δομή για το block ευρετηρίου 
  int Pointer;
  void *Key;
} indexblock;

void AM_Init( void );


int AM_CreateIndex(
  char *fileName, /* όνομα αρχείου */
  char attrType1, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength1, /* μήκος πρώτου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
  char attrType2, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength2 /* μήκος δεύτερου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
);


int AM_DestroyIndex(
  char *fileName /* όνομα αρχείου */
);


int AM_OpenIndex (
  char *fileName /* όνομα αρχείου */
);


int AM_CloseIndex (
  int fileDesc /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
);


int AM_InsertEntry(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  void *value1, /* τιμή του πεδίου-κλειδιού προς εισαγωγή */
  void *value2 /* τιμή του δεύτερου πεδίου της εγγραφής προς εισαγωγή */
);


int AM_OpenIndexScan(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  int op, /* τελεστής σύγκρισης */
  void *value /* τιμή του πεδίου-κλειδιού προς σύγκριση */
);


void *AM_FindNextEntry(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


int AM_CloseIndexScan(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


void AM_PrintError(
  char *errString /* κείμενο για εκτύπωση */
);

void AM_Close();


#endif /* AM_H_ */
