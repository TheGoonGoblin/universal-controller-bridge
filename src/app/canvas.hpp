// SPDX-License-Identifier: MIT
#pragma once
#include "settings.hpp"
#include <cstdint>
#include <cstring>
#include <vector>
namespace ucb::app {
// Small original bitmap alphabet: five pixels wide, seven rows.
struct Glyph { char c;const char* rows; };
inline const char* glyph(char c) {
  static const Glyph glyphs[]={
    {'A',"01110100011000111111100011000110001"},{'B',"11110100011000111110100011000111110"},
    {'C',"01111100001000010000100001000001111"},{'D',"11110100011000110001100011000111110"},
    {'E',"11111100001000011110100001000011111"},{'F',"11111100001000011110100001000010000"},
    {'G',"01111100001000010111100011000101111"},{'H',"10001100011000111111100011000110001"},
    {'I',"11111001000010000100001000010011111"},{'J',"00111000100001000010100101001001100"},
    {'K',"10001100101010011000101001001010001"},{'L',"10000100001000010000100001000011111"},
    {'M',"10001110111010110101100011000110001"},{'N',"10001110011010110011100011000110001"},
    {'O',"01110100011000110001100011000101110"},{'P',"11110100011000111110100001000010000"},
    {'Q',"01110100011000110001101011001001101"},{'R',"11110100011000111110101001001010001"},
    {'S',"01111100001000001110000010000111110"},{'T',"11111001000010000100001000010000100"},
    {'U',"10001100011000110001100011000101110"},{'V',"10001100011000110001100010101000100"},
    {'W',"10001100011000110101101011010101010"},{'X',"10001100010101000100010101000110001"},
    {'Y',"10001100010101000100001000010000100"},{'Z',"11111000010001000100010001000011111"},
    {'0',"01110100011001110101110011000101110"},{'1',"00100011000010000100001000010001110"},
    {'2',"01110100010000100010001000100011111"},{'3',"11110000010000101110000010000111110"},
    {'4',"00010001100101010010111110001000010"},{'5',"11111100001000011110000010000111110"},
    {'6',"01110100001000011110100011000101110"},{'7',"11111000010001000100010000100001000"},
    {'8',"01110100011000101110100011000101110"},{'9',"01110100011000101111000010000101110"},
    {'-',"00000000000000011111000000000000000"},{'.',"00000000000000000000000000011000110"},
    {':',"00000001100011000000001100011000000"},{'/',"00001000100001000100010000100010000"},
    {'%',"11001110100001000100010001011010011"},{'+',"00000001000010011111001000010000000"}
  };
  for(const auto& g:glyphs) if(g.c==c)return g.rows;
  return "00000000000000000000000000000000000";
}
struct Canvas {
  static constexpr int width=1920,height=1080;
  std::vector<uint32_t> pixels=std::vector<uint32_t>(width*height);
  static uint32_t rgb(unsigned r,unsigned g,unsigned b) { return 0xff000000u|(b<<16)|(g<<8)|r; }
  void rect(int x,int y,int w,int h,uint32_t color) {
    for(int j=y;j<y+h && j<height;++j)for(int i=x;i<x+w && i<width;++i)
      if(i>=0 && j>=0)pixels[size_t(j)*width+size_t(i)]=color;
  }
  void text(int x,int y,const char* s,int scale,uint32_t color) {
    for(;*s;++s,x+=6*scale) {
      const char* g=glyph(*s);
      for(int j=0;j<7;++j)for(int i=0;i<5;++i)if(g[j*5+i]=='1')rect(x+i*scale,y+j*scale,scale,scale,color);
    }
  }
  void draw(const Model& m) {
    const auto ink=rgb(230,238,251), muted=rgb(134,155,184), blue=rgb(69,164,255), green=rgb(87,226,179), amber=rgb(246,188,93);
    for(int y=0;y<height;++y)rect(0,y,width,1,rgb(10+unsigned(y)/120,17+unsigned(y)/100,31+unsigned(y)/90));
    rect(0,0,1920,7,blue);rect(48,48,94,94,blue);text(68,76,"UC",6,rgb(10,24,45));
    text(175,57,"UNIVERSAL CONTROLLER BRIDGE",6,ink);text(177,116,"PS5 NATIVE SETTINGS - EXPERIMENTAL 0.3",3,muted);
    const char* tabs[]{"OVERVIEW","SETTINGS","MAPPING","INPUT TEST"};
    for(int i=0;i<4;++i){rect(48,216+i*112,350,84,i==m.tab?rgb(27,77,121):rgb(18,30,48));text(76,244+i*112,tabs[i],4,i==m.tab?ink:muted);}
    rect(438,216,1434,704,rgb(20,34,54));
    text(484,260,tabs[m.tab],6,ink);
    if(m.tab==0) {
      text(484,354,"PLAYER 1 - SONY NATIVE PAD",4,ink);
      text(484,410,m.native_connected?"CONNECTED":"NOT DETECTED",4,m.native_connected?green:amber);
      text(484,508,"XBOX 360 USB",4,ink);text(484,564,"PS5 USB BACKEND NOT CONNECTED",4,amber);
      text(484,658,"EMULATOR LINK - NOT IMPLEMENTED",4,muted);
      text(484,734,"BACKGROUND SERVICE - NOT IMPLEMENTED",4,muted);
      text(484,826,"DECODER AND MAPPINGS ARE HOST TESTED",3,muted);
    } else if(m.tab==1) {
      const char* labels[]{"LEFT STICK DEAD ZONE","RIGHT STICK DEAD ZONE","XBOX PLAYER SLOT"};
      int values[]{m.settings.left,m.settings.right,m.settings.player};char value[32];
      for(int i=0;i<3;++i) {
        const int y=354+i*130;rect(470,y-20,1376,102,i==m.row?rgb(31,68,103):rgb(24,42,64));
        text(494,y,labels[i],4,ink);std::snprintf(value,sizeof(value),i==2?"%d":"%d %%",values[i]);text(1510,y,value,5,blue);
        if(i<2){rect(494,y+46,750,6,rgb(48,66,87));rect(494,y+46,values[i]*750/90,6,blue);}
      }
      text(484,788,"DPAD UP/DOWN SELECT - LEFT/RIGHT ADJUST",3,muted);
      text(484,844,"CROSS SAVES CONFIGURATION",3,green);
    } else if(m.tab==2) {
      const char* lines[]{"A / B / X / Y  -  CROSS / CIRCLE / SQUARE / TRIANGLE",
        "LB / RB  -  L1 / R1","STICK CLICKS  -  L3 / R3","START  -  OPTIONS",
        "BACK  -  TOUCHPAD FOR PS5X360","GUIDE  -  NO CURRENT GUEST MAPPING"};
      for(int i=0;i<6;++i)text(484,354+i*76,lines[i],3,i==5?amber:ink);
      text(484,844,"FIXED PROFILE - CUSTOM REMAPPING PLANNED",3,muted);
    } else {
      char buf[100];std::snprintf(buf,sizeof(buf),"NATIVE PAD BUTTON MASK: %u",m.buttons);text(484,354,buf,4,ink);
      const char* labels[]{"LX","LY","RX","RY","L2","R2"};int values[]{m.lx,m.ly,m.rx,m.ry,m.l2,m.r2};
      for(int i=0;i<6;++i){int y=440+i*58;text(484,y,labels[i],3,muted);rect(580,y,920,20,rgb(39,56,77));rect(580,y,values[i]*920/255,20,blue);std::snprintf(buf,sizeof(buf),"%d",values[i]);text(1550,y,buf,3,ink);}
      text(484,844,"LIVE SONY PAD INPUT - XBOX REQUIRES USB BACKEND",3,muted);
    }
    text(48,962,"L1/R1 TABS     CROSS SAVE     CIRCLE EXIT",3,ink);
    text(48,1014,m.message,3,muted);
  }
};
}
