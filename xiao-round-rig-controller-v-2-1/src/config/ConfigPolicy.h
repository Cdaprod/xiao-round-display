#pragma once
#include <cctype>
#include <cstdint>
#include <cstring>
namespace rig {
struct PlainConfig {char ssid[40]={},password[65]={},api[96]="http://192.168.0.25:8787",node[40]={},token[96]={};};
struct ConfigStatus {bool sdMounted=false,fileFound=false,fileParsed=false,wifi=false,node=false,token=false,api=false,overrides=false;};
inline bool validApiUrl(const char*s){return s&&(std::strncmp(s,"http://",7)==0||std::strncmp(s,"https://",8)==0)&&std::strlen(s)>8;}
inline void mergeConfig(PlainConfig&dst,const PlainConfig&src,uint8_t mask){char* d[]={dst.ssid,dst.password,dst.api,dst.node,dst.token};const char* v[]={src.ssid,src.password,src.api,src.node,src.token};size_t n[]={40,65,96,40,96};for(int i=0;i<5;i++)if(mask&(1u<<i)){std::strncpy(d[i],v[i],n[i]-1);d[i][n[i]-1]=0;}}
inline const char* redacted(bool set){return set?"SET":"NOT SET";}
inline ConfigStatus configStatus(const PlainConfig&c,bool mounted,bool found,bool parsed,bool overrides){return {mounted,found,parsed,c.ssid[0]&&std::strcmp(c.password,"CHANGE_ME"),c.node[0]!=0,c.token[0]!=0,validApiUrl(c.api),overrides};}
}
