#ifndef MOON_HINTS_PRESENTATION_ADAPTER_H
#define MOON_HINTS_PRESENTATION_ADAPTER_H
/* Uses this provider's observed Unity thread and live-world gates. This late
 * domain registry does not reopen the sealed loader callback registry. */
static unsigned mh_prepare_subscribe(const char *key,MoonHintsPrepare callback,void *user){
    if(!__atomic_load_n(&mh.started,__ATOMIC_ACQUIRE)||!mh.thread||mh.thread!=GetCurrentThreadId())return 0;
    return mhp_subscribe(&mh.presentation,key,callback,user);
}
static int mh_prepare_unsubscribe(unsigned handle){
    if(!mh.thread||mh.thread!=GetCurrentThreadId())return 0;
    return mhp_unsubscribe(&mh.presentation,handle);
}
static int mh_before_legend(PTR belt,int force){
    if(!__atomic_load_n(&mh.started,__ATOMIC_ACQUIRE)||!mh.thread||mh.thread!=GetCurrentThreadId())return 0;
    PTR walker=0,world=0;
    field_value(belt,mh.belt_walker,&walker);
    if(mh_allowed(walker,&world))mhp_dispatch(&mh.presentation,walker);
    return mh_controls(belt,force);
}
#endif
