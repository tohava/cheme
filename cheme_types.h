#define ANY_SIZE_THRESHOLD     16
#define ANY_SIZE_THRESHOLD_STR "16"



#define CHEME_TYPE_BOOL_STR \
    "typedef enum {false = 0      , true = 1      } bool;"
     typedef enum {cheme_false = 0, cheme_true = 1} cheme_bool;
#define CHEME_TYPE_UNIT_STR \
    "typedef struct {} unit;"
     typedef struct {} cheme_unit;
#define SS1 ANY_SIZE_THRESHOLD_STR
#define SS2 ANY_SIZE_THRESHOLD
struct cheme_type_desc_data {
	int size;
	char *name;
};
#define CHEME_TYPE_ANY_STR \
    "typedef struct { hidden_cheme_type_desc_data *t;  char s["SS1"]; char *d; } any;"
     typedef struct { cheme_type_desc_data *t; char s[ SS2 ]; char *d; } cheme_any;
#define CHEME_TYPE_SYM_STR \
    "typedef struct { char *str; } sym;"
     typedef struct { char *str; } cheme_sym;

	 
typedef struct cheme_anylist { struct { cheme_any car; cheme_anylist *cdr; } data; }
cheme_anylist;


	 
typedef cheme_type_desc_data *cheme_type_desc;
