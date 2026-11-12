/* Native item controls are an extension point, not a second HUD. */
#include "../../runtime/native/gameplay.h"
#include "../../runtime/native/services.h"
#include "registry.h"
#include "admission.h"
#include "presentation_registry.h"
#include "../sound-framework/sdk/client.h"
int _fltused=0;
typedef U64 MHRoot;
typedef struct {float x,y,z;} MHScale;
_Static_assert(sizeof(MHRoot)==sizeof(PTR),"current IL2CPP pointer-sized GC roots");
static struct {
    int started,owned,test;DWORD thread;unsigned int frame;
    MHRegistry registry;MHRoot walkers[MH_PROVIDER_LIMIT],worlds[MH_PROVIDER_LIMIT];
    MHPRegistry presentation;
    MHRoot belt,row,hints;unsigned int shown_revision;int shown_device,shown_provider;
    MHScale original_scale;int scale_captured,legend_key_offset;
    PTR world_field,belt_walker,legend_field,legend_key;
    PTR frame_get,focus,firstperson,blocked,typing,restoring,device;
    PTR selected,legend_for,keyrow_build,go_get,go_active,tr_scale,tr_get_scale;
    PTR tuple_class,string_class;
    PTR (*param)(PTR,DWORD),(*field_type)(PTR),(*return_type)(PTR),(*element_class)(PTR);
    const char *(*param_name)(PTR,DWORD),*(*class_name)(PTR),*(*class_namespace)(PTR);
    char *(*type_name)(PTR);void (*free)(PTR);
    PTR (*array_new)(PTR,U64),(*string_new)(const char *),(*target)(MHRoot);
    U64 (*array_length)(PTR);
    MHRoot (*root)(PTR,BOOL);void (*unroot)(MHRoot);
    int (*field_offset)(PTR),(*value_size)(PTR,DWORD *);
    void (*barrier)(PTR,PTR *,PTR);
    PTR implicit,log;
} mh;
static void mh_restore(void);
static void mh_visible(PTR row,int visible);
static MHRoot mh_hold(PTR p){return p?mh.root(p,0):0;}
static PTR mh_get(MHRoot h){return h?mh.target(h):0;}
static void mh_drop(MHRoot *h){if(*h)mh.unroot(*h);*h=0;}
static int mh_alive(PTR obj){int ok=1;void *a[]={obj};PTR b=obj?call(mh.implicit,0,a,&ok):0;return ok&&b&&*(unsigned char *)unbox(b);}
static void mh_event(const char *name){if(!mh.log)return;char b[192],*p=cat(b,"{\"event\":\"");p=cat(p,name);p=cat(p,"\",\"frame\":");p=num(p,mh.frame);p=cat(p,"}\n");DWORD n;WriteFile(mh.log,b,(DWORD)(p-b),&n,0);}
static void mh_trace(const char *stage){
    if(!mh.test)return;static const char *previous;static unsigned count;
    int active=0;for(int i=0;i<MH_PROVIDER_LIMIT;i++)active|=mh.registry.entries[i].active;
    if(!active||count>=32||previous==stage)return;previous=stage;count++;mh_event(stage);
}
static void mh_binding_failure(const char *expected,const char *actual){
    if(!mh.log)return;char b[1200],*p=cat(b,"{\"event\":\"binding-failed\",\"expected\":\"");
    const char *values[]={expected,actual?actual:"<missing>"};
    for(int i=0;i<2;i++){
        if(i)p=cat(p,"\",\"actual\":\"");
        for(unsigned n=0;values[i][n]&&n<500;n++){unsigned char c=(unsigned char)values[i][n];*p++=c<32||c=='"'||c=='\\'?'?':(char)c;}*p=0;
    }
    p=cat(p,"\"}\n");DWORD n;WriteFile(mh.log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(mh.log);
}
static int mh_type(PTR type,const char *expected){if(!type){mh_binding_failure(expected,0);return 0;}char *name=mh.type_name(type);int equal=name&&mh_equal(name,expected);if(!equal)mh_binding_failure(expected,name);if(name)mh.free(name);return equal;}
static int mh_signature(PTR method_id,DWORD token,const char **types,unsigned int count){
    if(!method_id||method_token(method_id)!=token){mh_event("method-token-check-failed");return 0;}
    for(unsigned int i=0;i<count;i++)if(!mh_type(mh.param(method_id,i),types[i]))return 0;
    return 1;
}
static int mh_frame(unsigned int *out){int ok=1;*out=(unsigned int)integer(mh.frame_get,0,&ok);return ok;}
static void mh_release_entry(int index){
    if(mh.owned&&mh.shown_provider==index){mh_visible(mh_get(mh.row),0);mh_restore();}
    mh_forget(mh.registry.entries+index);mh_drop(mh.walkers+index);mh_drop(mh.worlds+index);
}
static int mh_allowed(PTR walker,PTR *world){
    MHAdmissionSnapshot state={0};int ok=1;*world=0;
    state.live_walker=mh_alive(walker);if(!state.live_walker)return 0;
    field_value(walker,mh.world_field,world);
    state.live_world=mh_alive(*world);if(!state.live_world)return 0;
    state.focused=boolean(mh.focus,0,&ok);
    state.first_person=boolean(mh.firstperson,walker,&ok);
    state.menu_blocked=boolean(mh.blocked,walker,&ok);
    state.typing=boolean(mh.typing,0,&ok);
    state.restoring=boolean(mh.restoring,*world,&ok);
    state.queries_succeeded=ok;
    return mh_admission(&state)==MH_ADMIT_ALLOWED;
}
static int mh_publish(const char *key,void *walker,const char *controls){
    unsigned int frame=0;PTR world=0;MHControl parsed[MH_CONTROL_LIMIT];
    if(!mh.thread||mh.thread!=GetCurrentThreadId()||!mh_frame(&frame)||!mh_allowed(walker,&world)||mh_length(controls,MH_TEXT_BYTES)==MH_TEXT_BYTES||!mh_parse(controls,parsed))return 0;
    MHEntry *e=mh_record(&mh.registry,key,walker,controls,frame);if(!e)return 0;int index=(int)(e-mh.registry.entries);
    if(mh_get(mh.walkers[index])!=walker||mh_get(mh.worlds[index])!=world){
        mh_drop(mh.walkers+index);mh_drop(mh.worlds+index);mh.walkers[index]=mh_hold(walker);mh.worlds[index]=mh_hold(world);
        if(!mh.walkers[index]||!mh.worlds[index]){mh_release_entry(index);return 0;}
    }return 1;
}
static void mh_clear(const char *key){if(!mh.thread||mh.thread!=GetCurrentThreadId())return;MHEntry *e=mh_lookup(&mh.registry,key);if(e)mh_release_entry((int)(e-mh.registry.entries));}
static PTR mh_gameobject(PTR component){int ok=1;return call(mh.go_get,component,0,&ok);}
static void mh_visible(PTR row,int visible){int ok=1;unsigned char value=(unsigned char)!!visible;void *a[]={&value};call(mh.go_active,mh_gameobject(row),a,&ok);}
static void mh_restore(void){
    PTR belt=mh_get(mh.belt),row=mh_get(mh.row);
    if(mh.scale_captured&&mh_alive(row)){
        int ok=1;void *a[]={&mh.original_scale};call(mh.tr_scale,row,a,&ok);
    }
    mh.scale_captured=0;
    if(mh.owned&&mh_alive(belt)){
        /* Null invalidates the game's cached legend; vanilla reconstructs its
         * own row on the next normal update, including native device changes. */
        mh.barrier(belt,(PTR *)((unsigned char *)belt+mh.legend_key_offset),0);
    }
    mh.owned=0;mh.shown_revision=0;mh_drop(&mh.belt);mh_drop(&mh.row);mh_drop(&mh.hints);
}
/* Derive the inflated tuple class from a native LegendFor result. This avoids
 * guessing generic layouts or activating a reflection-created generic type. */
static int mh_tuple(PTR belt,PTR tool){
    if(mh.tuple_class)return 1;int ok=1;unsigned char no=0;void *a[]={tool,&no,&no};PTR list=call(mh.legend_for,belt,a,&ok);
    if(!ok||!list)return 0;MHRoot hold=mh_hold(list);if(!hold)return 0;
    PTR f=get_field(object_class(list),"_items"),array=0;if(f)field_value(list,f,&array);
    PTR cls=array?mh.element_class(object_class(array)):0;
    PTR k=cls?get_field(cls,"Item1"):0,t=cls?get_field(cls,"Item2"):0;DWORD align=0;
    int valid=k&&t&&mh_equal(mh.class_namespace(cls),"System")&&mh_equal(mh.class_name(cls),"ValueTuple`2")&&mh.field_offset(k)==16&&mh.field_offset(t)==24&&mh.value_size(cls,&align)==16&&mh_type(mh.field_type(k),"System.String[]")&&mh_type(mh.field_type(t),"System.String");
    mh_drop(&hold);if(valid)mh.tuple_class=cls;return valid;
}
static MHRoot mh_build_hints(const char *text){
    MHControl controls[MH_CONTROL_LIMIT];int count=mh_parse(text,controls);if(!count)return 0;
    PTR array=mh.array_new(mh.tuple_class,(U64)count);MHRoot hold=mh_hold(array);if(!hold)return 0;
    for(int i=0;i<count;i++){
        PTR keys=mh.array_new(mh.string_class,(U64)controls[i].keys);MHRoot key_hold=mh_hold(keys);if(!key_hold){mh_drop(&hold);return 0;}
        for(int k=0;k<controls[i].keys;k++){
            PTR key=mh.string_new(controls[i].key[k]);if(!key){mh_drop(&key_hold);mh_drop(&hold);return 0;}
            mh.barrier(keys,(PTR *)((unsigned char *)keys+32+8*k),key);
        }
        PTR *tuple=(PTR *)((unsigned char *)array+32+16*i);
        mh.barrier(array,tuple,keys);mh_drop(&key_hold);
        PTR label=mh.string_new(controls[i].label);if(!label){mh_drop(&hold);return 0;}
        mh.barrier(array,tuple+1,label);
    }return hold;
}
static int mh_controls(PTR belt,int visible){
    unsigned int frame;PTR walker=0,world=0,row=0;if(!mh_frame(&frame)){mh_trace("hint-check-frame-failed");return 0;}
    field_value(belt,mh.belt_walker,&walker);MHEntry *e=mh_current(&mh.registry,walker,frame);
    if(!e||!visible||!mh_allowed(walker,&world)||world!=mh_get(mh.worlds[e-mh.registry.entries])){mh_trace(!e?"hint-check-current-entry-failed":!visible?"hint-check-native-hidden":"hint-check-world-gate-failed");mh_restore();return 0;}
    field_value(belt,mh.legend_field,&row);if(!mh_alive(row)){mh_trace("hint-check-row-failed");mh_restore();return 0;}
    int ok=1,device=integer(mh.device,0,&ok);if(!ok){mh_trace("hint-check-device-failed");mh_restore();return 0;}
    if(mh_get(mh.belt)!=belt||mh_get(mh.row)!=row){mh_restore();mh.belt=mh_hold(belt);mh.row=mh_hold(row);if(!mh.belt||!mh.row){mh_restore();return 0;}}
    if(!mh.owned||mh.shown_revision!=e->revision||mh.shown_device!=device){
        PTR tool=call(mh.selected,belt,0,&ok);if(!ok||!tool||!mh_tuple(belt,tool)){mh_trace(!ok||!tool?"hint-check-tool-failed":"hint-check-tuple-failed");mh_restore();return 0;}
        MHRoot hints=mh_build_hints(e->text);if(!hints){mh_trace("hint-check-array-failed");mh_restore();return 0;}
        if(!mh.scale_captured){
            PTR scale=call(mh.tr_get_scale,row,0,&ok);
            if(!ok||!scale){mh_trace("hint-check-read-scale-failed");mh_drop(&hints);mh_restore();return 0;}
            mh.original_scale=*(MHScale *)unbox(scale);mh.scale_captured=1;
        }
        /* Native Build can replace children before failing; claim restoration
         * before entering it so even partial writes invalidate the cache. */
        mh.owned=1;mh.shown_provider=(int)(e-mh.registry.entries);
        void *a[]={row,mh_get(hints),0};PTR width=call(mh.keyrow_build,0,a,&ok);
        if(!ok||!width){mh_trace("hint-check-native-build-failed");mh_drop(&hints);mh_restore();return 0;}
        float w=*(float *)unbox(width),scale=.85f;
        if(w>0&&620.f/w<scale)scale=620.f/w;
        MHScale value={scale,scale,scale};void *s[]={&value};call(mh.tr_scale,row,s,&ok);
        if(!ok){mh_trace("hint-check-apply-scale-failed");mh_drop(&hints);mh_restore();return 0;}
        mh_drop(&mh.hints);mh.hints=hints;mh.shown_revision=e->revision;mh.shown_device=device;mh.owned=1;mh_event("native-item-controls-published");
    }
    mh_visible(row,1);return 1;
}
#include "presentation_adapter.h"
static int before(int id,Context *c){
    if(id==0){
        if(!mh.thread)mh.thread=GetCurrentThreadId();if(mh.thread!=GetCurrentThreadId())return 0;
        unsigned int frame;if(!mh_frame(&frame))return 0;mh.frame=frame;
        for(int i=0;i<MH_PROVIDER_LIMIT;i++){
            MHEntry *e=mh.registry.entries+i;if(!e->active)continue;
            PTR walker=mh_get(mh.walkers[i]),world=0;
            if((unsigned int)(frame-e->frame)>1||!mh_allowed(walker,&world)||world!=mh_get(mh.worlds[i]))mh_release_entry(i);
        }
        PTR walker=(PTR)c->rcx,world=0;
        if(mh.owned&&!mh_allowed(walker,&world)){mh_visible(mh_get(mh.row),0);mh_restore();}
        return 0;
    }
    if(mh.thread!=GetCurrentThreadId()){mh_trace("hint-check-thread-failed");return 0;}
    if(id==1&&mh_before_legend((PTR)c->rcx,(int)c->rdx&255)){c->rax=0;return 1;}
    return 0;
}
static void mh_update(PTR walker){if(!__atomic_load_n(&mh.started,__ATOMIC_ACQUIRE))return;Context c={0};c.rcx=(U64)walker;before(0,&c);}
static void mh_runtime_update(void *walker,void *user){(void)user;mh_update(walker);}
static const MoonHints mh_api={MH_ABI,sizeof(MoonHints),mh_publish,mh_clear};
static const MoonHintsPresentation mh_presentation_api={MHP_ABI,sizeof(MoonHintsPresentation),mh_prepare_subscribe,mh_prepare_unsubscribe};
__declspec(dllexport) const MoonHints *MoonLab_Hints_v1(unsigned int abi,unsigned int size){return __atomic_load_n(&mh.started,__ATOMIC_ACQUIRE)&&abi==MH_ABI&&size==sizeof(MoonHints)?&mh_api:0;}
__declspec(dllexport) const MoonHintsPresentation *MoonLab_HintsPresentation_v1(unsigned abi,unsigned size){return __atomic_load_n(&mh.started,__ATOMIC_ACQUIRE)&&abi==MHP_ABI&&size==sizeof(MoonHintsPresentation)?&mh_presentation_api:0;}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(__atomic_load_n(&mh.started,__ATOMIC_ACQUIRE))return 1;
    char test_flag[8];mh.test=GetEnvironmentVariableA("MOONLAB_TEST_TOOLS",test_flag,sizeof(test_flag))&&test_flag[0]=='1';
    char folder[1000],path[1100];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,sizeof(folder));if(n&&n<sizeof(folder)){cat(cat(path,folder),"\\hints-framework.jsonl");mh.log=CreateFileA(path,0x40000000,7,0,2,0x80,0);if(mh.log==(PTR)(U64)-1)mh.log=0;}
    mh_event("startup");if(!bind_runtime(domain,game)){mh_event("runtime-binding-failed");return 0;}
#define MX(F,N,T) mh.F=(T)GetProcAddress(game,"il2cpp_" N);if(!mh.F){mh_event("export-missing: " N);return 0;}
    MX(param,"method_get_param",PTR(*)(PTR,DWORD));MX(param_name,"method_get_param_name",const char *(*)(PTR,DWORD));MX(return_type,"method_get_return_type",PTR(*)(PTR));MX(type_name,"type_get_name",char *(*)(PTR));MX(free,"free",void(*)(PTR));
    MX(field_type,"field_get_type",PTR(*)(PTR));MX(field_offset,"field_get_offset",int(*)(PTR));MX(element_class,"class_get_element_class",PTR(*)(PTR));MX(value_size,"class_value_size",int(*)(PTR,DWORD *));MX(class_name,"class_get_name",const char *(*)(PTR));MX(class_namespace,"class_get_namespace",const char *(*)(PTR));
    MX(array_length,"array_length",U64(*)(PTR));MX(array_new,"array_new",PTR(*)(PTR,U64));MX(string_new,"string_new",PTR(*)(const char *));MX(root,"gchandle_new",MHRoot(*)(PTR,BOOL));MX(target,"gchandle_get_target",PTR(*)(MHRoot));MX(unroot,"gchandle_free",void(*)(MHRoot));MX(barrier,"gc_wbarrier_set_field",void(*)(PTR,PTR *,PTR));
#undef MX
    /* The current exported length producer pins array length at +0x18 and
     * therefore the checked native array payload begins at +0x20. */
    const unsigned char *code=(const unsigned char *)(PTR)mh.array_length;
    if(code[0]!=0xe9){mh_event("array-export-thunk-failed");return 0;}int displacement=0;
    for(int i=0;i<4;i++)((unsigned char *)&displacement)[i]=code[i+1];
    code+=5+displacement;
    if(code[0]!=0x8b||code[1]!=0x41||code[2]!=0x18||code[3]!=0xc3){mh_event("array-layout-producer-failed");return 0;}
#define MM(F,NS,T,N,A) mh.F=method(NS,T,N,A);if(!mh.F){mh_event("method-missing: " T "::" N);return 0;}
    MM(frame_get,"UnityEngine","Time","get_frameCount",0);MM(focus,"UnityEngine","Application","get_isFocused",0);MM(firstperson,"MoonCleaners","MoonWalker","get_IsFirstPerson",0);MM(blocked,"MoonCleaners","MoonWalker","get_MenuBlocked",0);MM(typing,"MoonCleaners","MoonInput","get_Typing",0);MM(restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering",0);MM(device,"MoonCleaners","MoonInput","get_DeviceVersion",0);
    MM(selected,"MoonCleaners","MoonToolbelt","get_SelectedTool",0);MM(legend_for,"MoonCleaners","MoonToolbelt","LegendFor",3);MM(keyrow_build,"MoonCleaners","MoonKeyRow","Build",3);MM(go_get,"UnityEngine","Component","get_gameObject",0);MM(go_active,"UnityEngine","GameObject","SetActive",1);MM(tr_scale,"UnityEngine","Transform","set_localScale",1);MM(tr_get_scale,"UnityEngine","Transform","get_localScale",0);MM(implicit,"UnityEngine","Object","op_Implicit",1);
#undef MM
    const char *lf[]={"MoonCleaners.MoonToolDefinition","System.Boolean","System.Boolean"};
    const char *kr[]={"UnityEngine.RectTransform","System.Collections.Generic.IList<System.ValueTuple<System.String[],System.String>>","System.Collections.Generic.Dictionary<System.String,System.ValueTuple<MoonCleaners.MoonScoreboardGlass,TMPro.TMP_Text>>"};
    PTR update_legend=method("MoonCleaners","MoonToolbelt","UpdateToolLegend",1);const char *ul[]={"System.Boolean"};
    if(!mh_signature(mh.legend_for,100671276,lf,3)||!mh_signature(mh.keyrow_build,100667741,kr,3)||!mh_signature(update_legend,100671275,ul,1)||!mh_type(mh.return_type(mh.keyrow_build),"System.Single"))return 0;
    PTR walker=find_class("MoonCleaners","MoonWalker"),belt=find_class("MoonCleaners","MoonToolbelt");mh.world_field=get_field(walker,"world");mh.belt_walker=get_field(belt,"walker");mh.legend_field=get_field(belt,"toolLegend");mh.legend_key=get_field(belt,"toolLegendKey");mh.string_class=find_class("System","String");
    if(!mh.world_field||!mh.belt_walker||!mh.legend_field||!mh.legend_key||!mh.string_class)return 0;
    mh.legend_key_offset=mh.field_offset(mh.legend_key);
    if(!mh_type(mh.return_type(mh.tr_get_scale),"UnityEngine.Vector3")||mh.legend_key_offset!=216||mh.field_offset(mh.belt_walker)!=144||mh.field_offset(mh.legend_field)!=208||!mh_type(mh.field_type(mh.legend_field),"UnityEngine.RectTransform")||!mh_type(mh.field_type(mh.legend_key),"System.String"))return 0;
    if(moon_runtime_loader_present()){
        if(!moon_runtime_subscribe_frame(mh_runtime_update,0)){mh_event("runtime-frame-unavailable");return 0;}
    }else{
        /* Preserve the standalone Sound/Units bridge. A second local Update
         * hook on its already-patched instruction rejects native hint consumers. */
        const MoonSounds *sounds=ma_connect(domain,game);
        if(!sounds||!sounds->on_update||!sounds->on_update(mh_update)){mh_event("update-dispatch-unavailable");return 0;}
    }
    if(!prepare_hook(update_legend,100671275,1,game)){mh_event("legend-hook-unavailable");return 0;}
    before_hook=before;if(!install_hooks())return 0;if(moon_runtime_loader_present()){
        const MoonRuntimeApiV1 *runtime=moon_runtime_connect();
        const MoonServiceLimitV1 limits[]={{"max_providers",MH_PROVIDER_LIMIT},{"max_controls",MH_CONTROL_LIMIT}};
        MoonServicePublicationLimitsV1 publication={.base={.size=sizeof(publication),.service_id="hints",.abi=MH_ABI,.api_size=sizeof(mh_api),.api=&mh_api},.limit_count=sizeof(limits)/sizeof(limits[0]),.reserved=0,.limits=limits};
        if(!runtime||!runtime->publish(&publication.base))return 0;
        const MoonServiceLimitV1 presentation_limits[]={{"max_subscriptions",MHP_LIMIT}};
        MoonServicePublicationLimitsV1 presentation={.base={.size=sizeof(presentation),.service_id="presentation",.abi=MHP_ABI,.api_size=sizeof(mh_presentation_api),.api=&mh_presentation_api},.limit_count=1,.reserved=0,.limits=presentation_limits};
        if(!runtime->publish(&presentation.base))return 0;
    }
    __atomic_store_n(&mh.started,1,__ATOMIC_RELEASE);mh_event("ready");return 1;
}
