ENTRY(00000000, 0, build_base_type("int"), sizeof(int), "int")
ENTRY(00000001, 1, build_uni_poly_type("ptr", build_base_type("char")),
      sizeof(char*), "(ptr char)")
ENTRY(00000002, 2, build_uni_poly_type("ptr", build_base_type("anylist")),
      sizeof(char*), "(ptr anylist)")
ENTRY(00000003, 3, build_base_type("sym"), sizeof(cheme_sym), "sym")
ENTRY(00000004, 4, build_base_type("unit"), sizeof(cheme_unit), "unit")
LASTENTRY(00000005, 5, build_base_type("char"), sizeof(char), "char")
