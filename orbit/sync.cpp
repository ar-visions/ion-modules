#include <orbit/sync.hpp>

///
Symbols Station::symbols = {
    { None,       "none"        },
    { Project,    "project"     },
    { Ping,       "ping"        },
    { Pong,       "pong"        },
    { Dependency, "dep"         },
    { Resource,   "res"         },
    { Delete,     "delete"      },
    { Sync,       "sync"        },
    { Products,   "products"    },
    { Delivery,   "delivery"    },
    { Activate,   "activate"    }, 
    { Deactivate, "deactivate"  },
    { ForceBuild, "force-build" },
    { ForceClean, "force-clean" }
};
