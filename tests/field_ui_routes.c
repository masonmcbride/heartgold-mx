/* Keep original C object interfaces out of Objective-C's BOOL ABI. The native
 * window smoke check shares the connected scenario planner with C tests. */
#include "compat/motion.h"
#include "field_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"Native UI route failed at %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);} } while(0)
#include "field_routes.h"
void hg_ui_test_approach(HGFieldApp *app,unsigned script) { hg_test_approach(app,script); }
