
// have to: using namespace
#define NOT_IN_STATE(x) !fsm_mgr.currentStateIs(x)
#define IN_STATE(x) fsm_mgr.currentStateIs(x)
#define TRIGGER(x) fsm_mgr.trigger(x)
#define PRINT_STATE(x) fsm_mgr.printState(x)

// bitwise
#define CHECK_SET(var, pos) ((var) & (1 << (pos))) // pos is bit mask (like 0x80, 0x40, 0x20, 0x10 etc)
#define CHECK_CLEAR(var, pos) ((var & pos) == 0)   // pos is bit mask (like 0x80)
