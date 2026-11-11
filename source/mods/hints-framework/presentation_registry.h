#ifndef MOON_HINTS_PRESENTATION_REGISTRY_H
#define MOON_HINTS_PRESENTATION_REGISTRY_H
#include "registry.h"
#include "sdk/presentation.h"
typedef struct {char key[MH_KEY_BYTES];MoonHintsPrepare callback;void *user;unsigned handle;} MHPEntry;
typedef struct {MHPEntry entries[MHP_LIMIT];unsigned next;int busy;} MHPRegistry;
static int mhp_key_valid(const char *key){
    if(!mh_key_valid(key))return 0;
    int start=1,dot=0;
    for(unsigned i=0;key[i];i++){
        char c=key[i];
        if(c=='.'){if(start||dot)return 0;dot=1;start=1;continue;}
        if(c=='-'){if(start)return 0;start=1;continue;}
        if(c<'a'||c>'z'){if(c<'0'||c>'9'||(dot&&start&&key[i-1]=='.'))return 0;}
        start=0;
    }
    return dot&&!start;
}
static unsigned mhp_subscribe(MHPRegistry *r,const char *key,MoonHintsPrepare callback,void *user){
    if(r->busy||!callback||!mhp_key_valid(key)||r->next==0xffffffffu)return 0;
    MHPEntry *free=0;
    for(unsigned i=0;i<MHP_LIMIT;i++){
        MHPEntry *e=r->entries+i;
        if(e->handle&&mh_equal(e->key,key))return 0;
        if(!e->handle&&!free)free=e;
    }
    if(!free)return 0;
    mh_copy(free->key,key,mh_length(key,MH_KEY_BYTES));
    free->callback=callback;free->user=user;free->handle=++r->next;return free->handle;
}
static int mhp_unsubscribe(MHPRegistry *r,unsigned handle){
    if(r->busy||!handle)return 0;
    for(unsigned i=0;i<MHP_LIMIT;i++)if(r->entries[i].handle==handle){r->entries[i]=(MHPEntry){0};return 1;}
    return 0;
}
static void mhp_dispatch(MHPRegistry *r,void *walker){
    if(r->busy||!walker)return;
    unsigned order[MHP_LIMIT],count=0;
    for(unsigned i=0;i<MHP_LIMIT;i++)if(r->entries[i].handle){
        unsigned at=count++;
        while(at&&r->entries[order[at-1]].handle>r->entries[i].handle){order[at]=order[at-1];at--;}
        order[at]=i;
    }
    r->busy=1;
    for(unsigned i=0;i<count;i++){MHPEntry *e=r->entries+order[i];e->callback(walker,e->user);}
    r->busy=0;
}
#endif
