#ifndef MOON_HINTS_ADMISSION_H
#define MOON_HINTS_ADMISSION_H
/* Semantic policy only: no Unity pointers, tokens, layout or public ABI. An
 * adapter must obtain a fresh snapshot from its own qualified bindings. Zero
 * initialization represents unknown state and must never grant admission. */
typedef struct {
    int live_walker,live_world,queries_succeeded;
    int focused,first_person,menu_blocked,typing,restoring;
} MHAdmissionSnapshot;
typedef enum {
    MH_ADMIT_ALLOWED,
    MH_ADMIT_NO_WALKER,
    MH_ADMIT_NO_WORLD,
    MH_ADMIT_QUERY_FAILED,
    MH_ADMIT_UNFOCUSED,
    MH_ADMIT_NOT_FIRST_PERSON,
    MH_ADMIT_MENU,
    MH_ADMIT_TYPING,
    MH_ADMIT_RESTORING
} MHAdmission;
static MHAdmission mh_admission(const MHAdmissionSnapshot *s){
    if(!s||!s->live_walker)return MH_ADMIT_NO_WALKER;
    if(!s->live_world)return MH_ADMIT_NO_WORLD;
    if(!s->queries_succeeded)return MH_ADMIT_QUERY_FAILED;
    if(!s->focused)return MH_ADMIT_UNFOCUSED;
    if(!s->first_person)return MH_ADMIT_NOT_FIRST_PERSON;
    if(s->menu_blocked)return MH_ADMIT_MENU;
    if(s->typing)return MH_ADMIT_TYPING;
    if(s->restoring)return MH_ADMIT_RESTORING;
    return MH_ADMIT_ALLOWED;
}
#endif
