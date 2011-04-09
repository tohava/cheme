#include "cheme_types.h"

extern "C" {

#define ENTRY(longid, id, expr, size, name) \
cheme_type_desc_data hidden_cheme_type_desc_data##longid##_data = {size, name};
#define LASTENTRY(longid, id, expr, size, name) ENTRY(longid, id, expr, size, name)
#include "base_types_table.h"
#undef LASTENTRY
#undef ENTRY

	int base_type_count =
#define ENTRY(longid, id, expr, size, name) +1
#define LASTENTRY(longid, id, expr, size, name) +1
#include "base_types_table.h"
#undef LASTENTRY
#undef ENTRY
	;
};
