
// have to: using namespace
#define NOT_IN_STATE(x) !fsm_mgr.currentStateIs(x)
#define IN_STATE(x) fsm_mgr.currentStateIs(x)
#define TRIGGER(x) fsm_mgr.trigger(x)
#define PRINT_STATE(x) fsm_mgr.printState(x)

// bitwise
#define CHECK_BIT_HIGH(var, pos) ((var) & (1 << (pos)))
#define CHECK_BIT_LOW(var, pos) ((var & pos) == 0) // pos is bit mask (like 0x80, 0x40, 0x20, 0x10 etc)
