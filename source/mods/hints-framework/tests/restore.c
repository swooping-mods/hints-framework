/* Link-time section collection drops unused platform startup/hook paths; the
 * actual native ownership-release code below runs against managed-call fakes. */
#define __declspec(X)
#define __stdcall
#include "../mod.c"
#include <assert.h>
#include <stdio.h>
static unsigned char fake_belt[256],fake_row[64];
static unsigned char alive=1;
static int scales,barriers,hides,unroots;
static MHScale restored;
DWORD GetCurrentThreadId(void){return 7;}
static PTR fake_target(MHRoot h){return (PTR)h;}
static void fake_unroot(MHRoot h){assert(h);unroots++;}
static PTR fake_unbox(PTR p){return p;}
static void fake_barrier(PTR owner,PTR *slot,PTR value){
    assert(owner==fake_belt&&slot==(PTR *)(fake_belt+216)&&!value);
    *slot=value;barriers++;
}
static PTR fake_invoke(PTR method_id,PTR self,void **args,PTR *exception){
    (void)exception;
    if(method_id==(PTR)1)return &alive;
    if(method_id==(PTR)2){assert(self==fake_row);return self;}
    if(method_id==(PTR)3){assert(self==fake_row&&*(unsigned char *)args[0]==0);hides++;return 0;}
    if(method_id==(PTR)4){assert(self==fake_row);restored=*(MHScale *)args[0];scales++;return 0;}
    assert(!"unexpected managed call");return 0;
}
static void prepare(void){
    mh.thread=7;mh.target=fake_target;mh.unroot=fake_unroot;invoke=fake_invoke;unbox=fake_unbox;
    mh.implicit=(PTR)1;mh.go_get=(PTR)2;mh.go_active=(PTR)3;mh.tr_scale=(PTR)4;mh.barrier=fake_barrier;
    mh.legend_key_offset=216;mh.belt=(MHRoot)fake_belt;mh.row=(MHRoot)fake_row;mh.hints=9;
    mh.original_scale=(MHScale){.71f,.72f,.73f};mh.scale_captured=1;mh.owned=1;mh.shown_provider=0;
    *(PTR *)(fake_belt+216)=(PTR)8;
    assert(mh_record(&mh.registry,"tool",(PTR)11,"Z: Zoom",10));
    mh.walkers[0]=11;mh.worlds[0]=12;
}
int main(void){
    prepare();mh_clear("tool");
    assert(hides==1&&scales==1&&barriers==1&&unroots==5);
    assert(restored.x==.71f&&restored.y==.72f&&restored.z==.73f);
    assert(!*(PTR *)(fake_belt+216)&&!mh.owned&&!mh.scale_captured&&!mh.row&&!mh.belt&&!mh.hints);
    assert(!mh.registry.entries[0].active&&!mh.walkers[0]&&!mh.worlds[0]);
    mh_restore();assert(scales==1&&barriers==1&&unroots==5);
    prepare();mh_release_entry(0); /* Same ownership path as Update expiry. */
    assert(scales==2&&barriers==2&&hides==2&&unroots==10);
    puts("hints restore: clear/expiry hide, exact scale restoration, null reference barrier and root release passed");
}
