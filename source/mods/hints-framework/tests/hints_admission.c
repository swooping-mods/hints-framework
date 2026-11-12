#include <assert.h>
#include <stdio.h>
#include "../admission.h"
int main(void){
    const MHAdmissionSnapshot ready={1,1,1,1,1,0,0,0};
    MHAdmissionSnapshot s={0};
    assert(mh_admission(0)==MH_ADMIT_NO_WALKER);
    assert(mh_admission(&s)==MH_ADMIT_NO_WALKER);
    assert(mh_admission(&ready)==MH_ADMIT_ALLOWED);
    s=ready;s.live_walker=0;assert(mh_admission(&s)==MH_ADMIT_NO_WALKER);
    s=ready;s.live_world=0;assert(mh_admission(&s)==MH_ADMIT_NO_WORLD);
    s=ready;s.queries_succeeded=0;assert(mh_admission(&s)==MH_ADMIT_QUERY_FAILED);
    s=ready;s.focused=0;assert(mh_admission(&s)==MH_ADMIT_UNFOCUSED);
    s=ready;s.first_person=0;assert(mh_admission(&s)==MH_ADMIT_NOT_FIRST_PERSON);
    s=ready;s.menu_blocked=1;assert(mh_admission(&s)==MH_ADMIT_MENU);
    s=ready;s.typing=1;assert(mh_admission(&s)==MH_ADMIT_TYPING);
    s=ready;s.restoring=1;assert(mh_admission(&s)==MH_ADMIT_RESTORING);
    /* Unknown queries cannot be interpreted as permissive false flags. */
    s=ready;s.queries_succeeded=0;s.menu_blocked=0;s.typing=0;s.restoring=0;
    assert(mh_admission(&s)==MH_ADMIT_QUERY_FAILED);
    puts("Hints admission: unknown and individual visibility faults fail closed");
    return 0;
}
