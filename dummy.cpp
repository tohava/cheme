#include "cheme_types.h"

extern "C" {
	struct hidden_cheme_type_desc_data {
		int size;
		char *name;
	};


	hidden_cheme_type_desc_data hidden_cheme_type_desc_datas[] = {
#define ENTRY(longid, id, expr, size, name) {size},
#define LASTENTRY(longid, id, expr, size, name) {size}
#include "base_types_table.h"
#undef LASTENTRY
#undef ENTRY
	};

	int base_type_count =
#define ENTRY(longid, id, expr, size, name) +1
#define LASTENTRY(longid, id, expr, size, name) +1
#include "base_types_table.h"
#undef LASTENTRY
#undef ENTRY
	;
#define ENTRY(longid, id, expr, size, name) \
    hidden_cheme_type_desc_data *hidden_cheme_type_desc_data##longid = \
    &hidden_cheme_type_desc_datas[id];
#define LASTENTRY ENTRY
#include "base_types_table.h"
#undef LASTENTRY
#undef ENTRY
};
