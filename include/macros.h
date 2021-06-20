
// have to: using namespace
#define NOT_IN_STATE(x) !fsm_mgr.currentStateIs(x)
#define IN_STATE(x) fsm_mgr.currentStateIs(x)
#define TRIGGER(x) fsm_mgr.trigger(x)
#define PRINT_STATE(x) fsm_mgr.printState(x)

// bitwise
#define BIT_HIGH(var, pos) ((var) & (1 << (pos)))
#define BIT_CHANGED(var1, var2, pos) ((var1 ^ var2) > 0 ? BIT_HIGH((var1 ^ var2), pos) : 0)
#define BIT_SET(var, pos) (var |= 1 << pos)
#define BIT_CLEAR(var, pos) (var &= ~(1 << pos))
#define BIT_TOGGLE(var, pos) (var ^= 1 << pos)