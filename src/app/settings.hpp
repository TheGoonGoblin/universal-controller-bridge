// SPDX-License-Identifier: MIT
#pragma once
#include <cstdio>
namespace ucb::app {
struct Settings { int left=15,right=15,player=2; };
inline bool valid(const Settings& s) { return s.left>=0 && s.left<=90 && s.right>=0 && s.right<=90 && s.player>=2 && s.player<=4; }
inline bool load(const char* path,Settings& settings) {
  FILE* f=std::fopen(path,"r");if(!f) return false;
  Settings next;char tail;
  const int count=std::fscanf(f,"UCB1 %d %d %d %c",&next.left,&next.right,&next.player,&tail);
  std::fclose(f);if(count!=3 || !valid(next)) return false;
  settings=next;return true;
}
inline bool save(const char* path,const Settings& settings) {
  if(!valid(settings)) return false;
  FILE* f=std::fopen(path,"w");if(!f) return false;
  bool ok=std::fprintf(f,"UCB1 %d %d %d\n",settings.left,settings.right,settings.player)>0;
  if(std::fclose(f)!=0) ok=false;
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
