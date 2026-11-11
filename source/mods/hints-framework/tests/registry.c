#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../registry.h"
int main(void){
    MHRegistry registry={0};void *a=(void *)1,*b=(void *)2;
    char text[]="Z: Zoom | Mouse1: Apply";
    MHEntry *e=mh_record(&registry,"zoom-module",a,text,11);assert(e);
    text[3]='x';assert(!strcmp(e->text,"Z: Zoom | Mouse1: Apply"));
    unsigned int revision=e->revision;assert(mh_record(&registry,"zoom-module",a,"Z: Zoom | Mouse1: Apply",11)==e);assert(e->revision==revision);
    assert(mh_current(&registry,a,11)==e);assert(!mh_current(&registry,b,11));assert(!mh_current(&registry,a,12));
    MHEntry *second=mh_record(&registry,"vacuum",a,"Mouse1: Sweep",11);assert(second&&mh_current(&registry,a,11)==second);
    mh_forget(second);assert(mh_current(&registry,a,11)==e);
    assert(mh_record(&registry,"zoom-module",b,"Z: Zoom",12)==e);assert(e->revision>revision&&mh_current(&registry,b,12)==e);
    assert(!mh_record(&registry,"bad key",a,"X: action",12));assert(!mh_record(&registry,"uppercase-KEY",a,"X: action",12));
    char huge[MH_TEXT_BYTES+1];memset(huge,'a',sizeof(huge));huge[sizeof(huge)-1]=0;assert(!mh_record(&registry,"zoom-module",a,huge,12));assert(e->walker==b);
    MHControl controls[MH_CONTROL_LIMIT];int count=mh_parse(" Z : Hold zoom | Ctrl + Mouse1 : Apply\nBattery 90%",controls);
    assert(count==3&&controls[0].keys==1&&!strcmp(controls[0].key[0],"Z")&&!strcmp(controls[0].label,"Hold zoom"));
    assert(controls[1].keys==2&&!strcmp(controls[1].key[0],"Ctrl")&&!strcmp(controls[1].key[1],"Mouse1"));
    assert(controls[2].keys==0&&!strcmp(controls[2].label,"Battery 90%"));
    assert(!mh_parse("1:A|2:B|3:C|4:D|5:E|6:F|7:G|8:H|9:I",controls));
    assert(!mh_parse("A+B+C+D+E: action",controls));assert(mh_parse(" | \n ",controls)==0);
    for(int i=2;i<MH_PROVIDER_LIMIT;i++){char key[64];snprintf(key,sizeof(key),"item-%d",i);assert(mh_record(&registry,key,a,"X: action",20));}
    assert(!mh_record(&registry,"overflow",a,"X: action",20));
    assert(mh_key_valid("namespace.item_2"));assert(!mh_key_valid(0));
    assert(sizeof(MoonHints)==2*sizeof(unsigned int)+2*sizeof(void *));
    puts("hints registry: provider isolation, borrowed-copy safety, frame expiry, bounded grammar and capacity passed");
}
