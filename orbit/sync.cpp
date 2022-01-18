#include <ux/sync.hpp>

///
Symbols StationCommand::symbols = {
    { None,       "none"        },
    { Project,    "project"     },
    { Ping,       "ping"        },
    { Pong,       "pong"        },
    { Dependency, "dep"         },
    { Resource,   "res"         },
    { Products,   "products"    },
    { Activate,   "activate"    }, 
    { Deactivate, "deactivate"  },
    { ForceBuild, "force-build" },
    { ForceClean, "force-clean" }
};
