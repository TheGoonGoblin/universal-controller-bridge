// SPDX-License-Identifier: MIT
#pragma once
#include <cstdio>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
namespace ucb::app {
struct Settings { int left=15,right=15,player=2; };
inline bool valid(const Settings& s) { return s.left>=0 && s.left<=90 && s.right>=0 && s.right<=90 && s.player>=2 && s.player<=4; }
inline bool load(const char* path,Settings& settings) {
  FILE* f=std::fopen(path,"r");if(!f) return false;
  // Bound input and reject embedded NULs or overflowing integer fields.
  char buffer[128]{};
  const size_t length=std::fread(buffer,1,sizeof(buffer)-1,f);
  bool ok=!std::ferror(f) && std::fgetc(f)==EOF;
  if(std::fclose(f)!=0)ok=false;
  if(!ok || length<4 || std::memchr(buffer,0,length) || std::strncmp(buffer,"UCB1",4)!=0)return false;
  Settings next;int* fields[]{&next.left,&next.right,&next.player};
  const char* cursor=buffer+4;
  for(auto* field:fields) {
    if(!std::isspace(static_cast<unsigned char>(*cursor)))return false;
    while(std::isspace(static_cast<unsigned char>(*cursor)))++cursor;
    char* end=nullptr;errno=0;
    const long value=std::strtol(cursor,&end,10);
    if(end==cursor || errno==ERANGE || value<0 || value>90)return false;
    *field=static_cast<int>(value);cursor=end;
  }
  while(std::isspace(static_cast<unsigned char>(*cursor)))++cursor;
  if(*cursor || !valid(next))return false;
  settings=next;return true;
}
inline bool save(const char* path,const Settings& settings) {
  if(!valid(settings)) return false;
  // Commit by rename only after the complete new file has closed successfully.
  // A failed write must not truncate the user's last saved configuration.
  char temporary[4096];
  const int n=std::snprintf(temporary,sizeof(temporary),"%s.tmp",path);
  if(n<0 || static_cast<size_t>(n)>=sizeof(temporary))return false;
  FILE* f=std::fopen(temporary,"w");if(!f) return false;
  bool ok=std::fprintf(f,"UCB1 %d %d %d\n",settings.left,settings.right,settings.player)>0;
  if(std::fclose(f)!=0) ok=false;
  if(ok && std::rename(temporary,path)!=0)ok=false;
  if(!ok)std::remove(temporary);
  return ok;
}
struct Model {
  Settings settings;
  int tab=0,row=0;
  bool native_connected=false;
  unsigned buttons=0;
  unsigned char lx=128,ly=128,rx=128,ry=128,l2=0,r2=0;
  const char* message="SETTINGS APPLY TO FUTURE BRIDGE SESSIONS";
  void change(int direction) {
    int& v=row==0?settings.left:row==1?settings.right:settings.player;
    const int low=row==2?2:0, high=row==2?4:90;
    v+=direction;if(v<low)v=low;if(v>high)v=high;
    message="UNSAVED CHANGES";
  }
};
}
