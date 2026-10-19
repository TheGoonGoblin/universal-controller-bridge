/*
 * Adapted from ps5-payload-dev/SDL SDL_ps5tilemap.c; scalar UCB variant.
 * Copyright (C) 2026 John Tornblom <john.tornblom@gmail.com>
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If used in a product, an
 *    acknowledgment in the product documentation would be appreciated but
 *    is not required.
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#pragma once
#include <cstdint>
namespace ucb::ps5 {
inline uint32_t tile_offset(uint32_t x,uint32_t y) {
  return ((x&1)<<0)|(((x>>1)&1)<<1)|((y&1)<<2)|(((y>>1)&1)<<3)|
    (((y>>2)&1)<<4)|(((x>>2)&1)<<5)|((((x>>3)^(y>>3))&1)<<6)|
    ((((x>>4)^(y>>4))&1)<<7)|((((x>>6)^(y>>5))&1)<<8)|
    ((((x>>5)^(y>>6))&1)<<9)|(((y>>3)&1)<<10)|(((x>>4)&1)<<11)|
    (((y>>6)&1)<<12)|(((x>>6)&1)<<13)|(((x>>7)&1)<<14)|(((x>>8)&1)<<15);
}
inline uint32_t tile_pixel(uint32_t x,uint32_t y,uint32_t width) {
  return (x/512)*65536+(y/128)*(128*width)+(tile_offset(x%512,0)^tile_offset(0,y%128));
}
}
