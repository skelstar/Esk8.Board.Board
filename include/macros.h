
// have to: using namespace
#define NOT_IN_STATE(x) !fsm_mgr.currentStateIs(x)
#define IN_STATE(x) fsm_mgr.currentStateIs(x)
#define TRIGGER(x) fsm_mgr.trigger(x)
#define PRINT_STATE(x) fsm_mgr.printState(x)
