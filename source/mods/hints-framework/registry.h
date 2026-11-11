#ifndef MOON_HINTS_REGISTRY_H
#define MOON_HINTS_REGISTRY_H
#include "sdk/hints.h"
#define MH_KEY_BYTES 64
#define MH_TEXT_BYTES 768
#define MH_LABEL_BYTES 160
#define MH_KEYCAP_BYTES 48
#define MH_COMBO_LIMIT 4
typedef struct {
    char key[MH_KEY_BYTES], text[MH_TEXT_BYTES];
    void *walker;
    unsigned int frame, revision;
    int active;
} MHEntry;
typedef struct { MHEntry entries[MH_PROVIDER_LIMIT]; unsigned int revision; } MHRegistry;
typedef struct {
    char key[MH_COMBO_LIMIT][MH_KEYCAP_BYTES], label[MH_LABEL_BYTES];
    int keys;
} MHControl;
static int mh_equal(const char *a,const char *b) {
    while (*a && *a==*b) {a++;b++;} return *a==*b;
}
static unsigned int mh_length(const char *s,unsigned int limit) {
    unsigned int n=0;if(!s)return limit;while(n<limit&&s[n])n++;return n;
}
static void mh_copy(char *out,const char *in,unsigned int n) {
    for(unsigned int i=0;i<n;i++)out[i]=in[i];out[n]=0;
}
static int mh_key_valid(const char *key) {
    unsigned int n=mh_length(key,MH_KEY_BYTES);if(!n||n==MH_KEY_BYTES)return 0;
    for(unsigned int i=0;i<n;i++)if(!((key[i]>='a'&&key[i]<='z')||(key[i]>='0'&&key[i]<='9')||key[i]=='-'||key[i]=='_'||key[i]=='.'))return 0;
    return 1;
}
static MHEntry *mh_lookup(MHRegistry *r,const char *key) {
    if(!mh_key_valid(key))return 0;
    for(int i=0;i<MH_PROVIDER_LIMIT;i++)if(mh_equal(r->entries[i].key,key))return &r->entries[i];
    return 0;
}
static MHEntry *mh_record(MHRegistry *r,const char *key,void *walker,const char *text,unsigned int frame) {
    unsigned int n=mh_length(text,MH_TEXT_BYTES);
    if(!walker||!mh_key_valid(key)||!n||n==MH_TEXT_BYTES)return 0;
    MHEntry *e=mh_lookup(r,key);
    if(!e)for(int i=0;i<MH_PROVIDER_LIMIT;i++)if(!r->entries[i].key[0]){e=&r->entries[i];mh_copy(e->key,key,mh_length(key,MH_KEY_BYTES));break;}
    if(!e)return 0;
    if(!e->active||e->walker!=walker||!mh_equal(e->text,text)){e->revision=++r->revision;mh_copy(e->text,text,n);}
    e->walker=walker;e->frame=frame;e->active=1;return e;
}
static void mh_forget(MHEntry *e) {if(e){e->active=0;e->walker=0;}}
static MHEntry *mh_current(MHRegistry *r,void *walker,unsigned int frame) {
    MHEntry *chosen=0;
    for(int i=0;i<MH_PROVIDER_LIMIT;i++){
        MHEntry *e=&r->entries[i];
        if(e->active&&e->walker==walker&&e->frame==frame&&(!chosen||e->revision>chosen->revision))chosen=e;
    }return chosen;
}
static int mh_part(char *out,unsigned int cap,const char *begin,const char *end) {
    while(begin<end&&(*begin==' '||*begin=='\t'))begin++;
    while(end>begin&&(end[-1]==' '||end[-1]=='\t'||end[-1]=='\r'))end--;
    unsigned int n=(unsigned int)(end-begin);if(n>=cap)return 0;mh_copy(out,begin,n);return 1;
}
static int mh_parse(const char *text,MHControl controls[MH_CONTROL_LIMIT]) {
    const char *p=text;int count=0;
    while(*p){
        const char *start=p,*colon=0;while(*p&&*p!='|'&&*p!='\n'){if(*p==':'&&!colon)colon=p;p++;}
        const char *end=p;if(*p)p++;
        while(start<end&&(*start==' '||*start=='\t'))start++;
        if(start==end)continue;if(count==MH_CONTROL_LIMIT)return 0;
        MHControl *c=controls+count;c->keys=0;
        if(colon){
            const char *k=start;
            while(k<colon){const char *stop=k;while(stop<colon&&*stop!='+')stop++;
                if(c->keys==MH_COMBO_LIMIT||!mh_part(c->key[c->keys],MH_KEYCAP_BYTES,k,stop))return 0;
                if(c->key[c->keys][0])c->keys++;
                k=stop<colon?stop+1:stop;
            }
            start=colon+1;
        }
        if(!mh_part(c->label,MH_LABEL_BYTES,start,end))return 0;
        if(c->label[0])count++;
    }return count;
}
#endif
