#include <orbit/sync.hpp>

///
/// i want to be clear that i dont like enums in two places... this is the best i can come up with that has introspection
/// the alternative costs performance in constructor.  i need ideas here, but mostly C++ needs enums lets be frank.
Symbols Station::symbols = {
    { None,             "none"              },
    { Project,          "project"           },
    { Ping,             "ping"              },
    { Pong,             "pong"              },
    { Repo,             "repo"              },
    { Delete,           "delete"            },
    { Products,         "products"          },
    { SyncInbound,      "sync-inbound"      },
    { SyncPacket,       "sync-packet"       },
    { SyncComplete,     "sync-complete"     }, /// sync-complete, delivery-complete.  one from client one from server.
    { DeliveryInbound,  "delivery-inbound"  },
    { DeliveryPacket,   "delivery-packet"   },
    { DeliveryComplete, "delivery-complete" },
    { AutoBuild,        "auto-build"        },
    { ForceBuild,       "force-build"       },
    { ForceClean,       "force-clean"       }
};
