#include "builtin_func_info.h"

const builtin_func_t builtin_func_table[] = {
    { ADD,     "+"       },
    { SUB,     "-"       },
    { MUL,     "*"       },
    { DIV,     "/"       },
    { POW,     "^"       },

    { LOG,     "log"     },
    { LN,      "ln"      },
    { EXP,     "exp"     },
    
    { SIN,     "sin"     },
    { COS,     "cos"     },
    { TAN,     "tan"     },
    { CTAN,    "ctan"    },

    { ARCSIN,  "arcsin"  },
    { ARCCOS,  "arccos"  },
    { ARCTAN,  "arctan"  },
    { ARCCTAN, "arcctan" },

    { CH,      "ch"      },
    { SH,      "sh"      },

    { ARCSH,   "arcsh"   },
    { ARCCH,   "arcch"   }
};

const size_t builtin_func_table_size =
    sizeof(builtin_func_table) / sizeof(builtin_func_table[0]);