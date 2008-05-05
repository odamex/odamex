#ifndef PIXELMATH_H
#define PIXELMATH_H

#define PIXELMATH_MACRO
#ifdef PIXELMATH_MACRO


// Adds two pixels, 8bit-8bit-8bit-8bit
// stores result in d
inline void ADDPX32(Uint32 &d, Uint32 &s)
{
   Uint32 px, c;
   c = (d & 0x00ff00ff) + (s & 0x00ff00ff);
   px = (c & 0x00ff00ff) | ((c & 0x100) - ((c & 0x100) >> 8)) | ((c & 0x01000000) - ((c & 0x01000000) >> 8));
   c = ((d & 0xff00ff00) >> 8) + ((s & 0xff00ff00) >> 8);
   px |= ((c & 0x00ff00ff) | ((c & 0x100) - ((c & 0x100) >> 8)) | ((c & 0x01000000) - ((c & 0x01000000) >> 8))) << 8;
   d = px;
}


inline void ADDPX32old(Uint32 &d, Uint32 &s)
{
   Uint32 px, c;
   c  = ((d & 0xff) + (s & 0xff));
   px = c > 0xff ? 0xff : c;
   c  = ((d & 0xff00) + (s & 0xff00));
   px |= c > 0xff00 ? 0xff00 : c;
   c  = ((d & 0xff0000) + (s & 0xff0000));
   px |= c > 0xff0000 ? 0xff0000 : c;
   c  = ((d >> 24) + (s >> 24));
   px |= c > 0xff ? 0xff000000 : (c << 24);
   d = px;
}



// Adds two pixels, 8bit-8bit-8bit
// stores result in d
inline void ADDPX24(Uint32 &d, Uint32 &s)                             
{
   Uint32 px = (d & 0xff000000), c;
   c  = ((d & 0xff) + (s & 0xff));
   px |= c > 0xff ? 0xff : c;
   c  = ((d & 0xff00) + (s & 0xff00));
   px |= c > 0xff00 ? 0xff00 : c;
   c  = ((d & 0xff0000) + (s & 0xff0000));
   px |= c > 0xff0000 ? 0xff0000 : c;
   d = px;
}

// Adds two pixels, 5bit-6bit-5bit
// stores result in d
inline void ADDPX16(Uint16 &d, Uint16 &s)                             
{
   Uint16 px, c;
   c  = ((d & 0x1F) + (s & 0x1F));
   px = c > 0x1F ? 0x1F : c;
   c  = ((d & 0x07E0) + (s & 0x07E0));
   px |= c > 0x07E0 ? 0x07E0 : c;
   c  = ((d >> 11) + (s >> 11));
   px |= c > 0x1F ? 0xF800 : (c << 11);
   d = px;
}



// Adds a value to each component of a pixel, 8bit-8bit-8bit-8bit
// stores result in d
inline void ADDTOPX32(Uint32 &d, Uint32 &v)                   
{
   Uint32 px, c;
   c  = ((d & 0xff) + v);
   px = c > 0xff ? 0xff : c;
   c  = ((d & 0xff00) + (v << 8));
   px |= c > 0xff00 ? 0xff00 : c;
   c  = ((d & 0xff0000) + (v << 16));
   px |= c > 0xff0000 ? 0xff0000 : c;
   c  = ((d >> 24) + v);
   px |= c > 0xff ? 0xff000000 : (c << 24);
   d = px;
}

// Adds a value to each component of a pixel, 8bit-8bit-8bit
// stores result in d
inline void ADDTOPX24(Uint32 &d, Uint32 &v)                   
{
   Uint32 px = (d & 0xff000000), c;
   c  = ((d & 0xff) + v);
   px |= c > 0xff ? 0xff : c;
   c  = ((d & 0xff00) + (v << 8));
   px |= c > 0xff00 ? 0xff00 : c;
   c  = ((d >> 16) + v);
   px |= c > 0xff ? 0xff0000 : (c << 16);
   d = px;
}

// Adds a value to each component of a pixel, 5bit-6bit-5bit
// stores result in d
inline void ADDTOPX16(Uint16 &d, Uint16 &v)                   
{
   Uint16 px, c;
   c  = ((d & 0x1F) + v);
   px = c > 0x1F ? 0x1F : c;
   c  = ((d & 0x07E0) + (v << 5));
   px |= c > 0x07E0 ? 0x07E0 : c;
   c  = ((d >> 11) + v);
   px |= c > 0x1F ? 0xF800 : (c << 11);
   d = px;
}




// Subtracts two pixels, 8bit-8bit-8bit-8bit
// stores result in d
inline void SUBPX32(Uint32 &d, Uint32 &s)
{
   Uint32 px, c;

   c  = ((d & 0xff) - (s & 0xff)) & 0xff;
   px = c > (d & 0xff) ? 0 : c;
   c  = ((d & 0xff00) - (s & 0xff00)) & 0xff00;
   px |= c > (d & 0xff00) ? 0 : c;
   c  = ((d & 0xff0000) - (s & 0xff0000)) & 0xff0000;
   px |= c > (d & 0xff0000) ? 0 : c;
   c  = (d - s) & 0xff000000;
   px |= c > d ? 0 : c;
   d = px;
}

// Subtracts two pixels, 8bit-8bit-8bit
// stores result in d
inline void SUBPX24(Uint32 &d, Uint32 &s)                                          
{
   Uint32 px = (d & 0xff000000), c;

   c  = ((d & 0xff) - (s & 0xff)) & 0xff;
   px |= c > (d & 0xff) ? 0 : c;
   c  = ((d & 0xff00) - (s & 0xff00)) & 0xff00;
   px |= c > (d & 0xff00) ? 0 : c;
   c  = (d - s) & 0xff0000;
   px |= c > (d & 0xff0000) ? 0 : c;
   d = px;
}

// Subtracts two pixels, 5bit-6bit-5bit
// stores result in d
inline void SUBPX16(Uint16 &d, Uint16 &s)
{
   Uint16 px, c;
   c  = ((d & 0x1F) - (s & 0x1F)) & 0x1F;
   px = c > (d & 0x1F) ? 0 : c;
   c  = ((d & 0x07E0) - (s & 0x07E0)) & 0x07E0;
   px |= c > (d & 0x07E0) ? 0 : c;
   c  = (d - s) & 0xF800;
   px |= c > (d & 0xF800) ? 0 : c;
   d = px;
}



// Fast scales light from p2 and stores the result in p1
inline void LIGHTPX32(Uint32 &p1, Uint32 &p2, Uint32 &light)
{
   p1 = ((((p2 & 0x00ff00ff) * (light & 0xff)) >> 8) & 0xff00ff)
      | ((((p2 & 0xff00ff00) >> 8) * (light & 0xff)) & 0xff00ff00);
}

// Fast scales light from p2 and stores the result in p1
inline void LIGHTPX24(Uint32 &p1, Uint32 &p2, Uint32 &light)                                          
{
   p1 &= 0xff000000;
   p1 |= ((((p2 & 0x00ff00ff) * (light & 0xff)) >> 8) & 0xff00ff)
       | ((((p2 & 0x0000ff00) >> 8) * (light & 0xff)) & 0x0000ff00);
}

// fast scales a 16-bit pixel, 5bit-6bit-5bit
// stores result in d
inline void LIGHTPX16(Uint16 &p1, Uint16 &p2, Uint16 &light)
{
   p1 = (((p2 & 0x1F) * light) >> 6)
      | ((((p2 & 0x07E0) * light) >> 6) & 0x7E0)
      | ((((p2 >> 11) * light) & 0x7C0) << 5);
}




// Subtracts a value from all comonents of a pixel, 8bit-8bit-8bit-8bit
// stores result in d
inline void SUBFROMPX32(Uint32 &d, Uint32 &v)
{
   Uint32 px, c;

   c  = ((d & 0xff) - v) & 0xff;
   px = c > (d & 0xff) ? 0 : c;
   c  = ((d & 0xff00) - (v  << 8)) & 0xff00;
   px |= c > (d & 0xff00) ? 0 : c;
   c  = ((d & 0xff0000) - (v << 16)) & 0xff0000;
   px |= c > (d & 0xff0000) ? 0 : c;
   c  = (d - (v << 24)) & 0xff000000;
   px |= c > d ? 0 : c;
   d = px;
}

// Subtracts a value from all comonents of a pixel, 8bit-8bit-8bit
// stores result in d
inline void SUBFROMPX24(Uint32 &d, Uint32 &v)
{
   Uint32 px = (d & 0xff000000), c;

   c  = ((d & 0xff) - v) & 0xff;
   px |= c > (d & 0xff) ? 0 : c;
   c  = ((d & 0xff00) - (v  << 8)) & 0xff00;
   px |= c > (d & 0xff00) ? 0 : c;
   c  = (d - (v << 16)) & 0xff0000;
   px |= c > (d & 0xff0000) ? 0 : c;
   d = px;
}

// Subtracts a value from all comonents of a pixel, 5bit-6bit-5bit
// stores result in d
inline void SUBFROMPX16(Uint16 &d, Uint16 &v)
{
   Uint16 px, c;
   c  = ((d & 0x1F) - (v & 0x1F)) & 0x1F;
   px = c > (d & 0x1F) ? 0 : c;
   c  = ((d & 0x07E0) - (v << 5)) & 0x07E0;
   px |= c > (d & 0x07E0) ? 0 : c;
   c  = (d - (v << 11)) & 0xF800;
   px |= c > (d & 0xF800) ? 0 : c;
   d = px;
}



// Multiplies a pixel by an int value, 8bit-8bit-8bit-8bit
// stores result in d
inline void MULPX32i(Uint32 &d, Uint32 &v)
{
   Uint32 px, c;
   c  = ((d & 0xff) * v);
   px = (c & 0xff) | ((c & 0x100) - ((c & 0x100) >> 8));
   c  = (((d >> 8) & 0xff) * v);
   px |= ((c & 0xff) | ((c & 0x100) - ((c & 0x100) >> 8))) << 8;
   c  = (((d >> 16) & 0xff) * v);
   px |= ((c & 0xff) | ((c & 0x100) - ((c & 0x100) >> 8))) << 16;
   c  = (((d >> 24) & 0xff) * v);
   px |= ((c & 0xff) | ((c & 0x100) - ((c & 0x100) >> 8))) << 24;
   d = px;
}



// Multiplies a pixel by an int value, 8bit-8bit-8bit
// stores result in d
inline void MULPX24i(Uint32 &d, Uint32 &v)
{
   Uint32 px = (d & 0xff000000), c;
   c  = ((d & 0xff) * v);
   px |= (c & 0xff) | ((c & 0x100) - ((c & 0x100) >> 8));
   c  = (((d >> 8) & 0xff) * v);
   px |= ((c & 0xff) | ((c & 0x100) - ((c & 0x100) >> 8))) << 8;
   c  = (((d >> 16) & 0xff) * v);
   px |= ((c & 0xff) | ((c & 0x100) - ((c & 0x100) >> 8))) << 16;
   d = px;
}

// Multiplies a pixel by an int value, 5bit-6bit-5bit
// stores result in d
inline void MULPX16i(Uint16 &d, Uint16 &v)
{
   Uint16 px, c;
   c  = ((d & 0x1F) * v);
   px = c > 0x1F ? 0x1F : c;
   c  = ((d & 0x07E0) * (v  << 5));
   px |= c > 0x07E0 ? 0x07E0 : c;
   c  = ((d >> 11) * v);
   px |= c > 0x1F ? 0xF800 : (c << 11);
   d = px;
}



// applies translucency, 8bit-8bit-8bit-8bit
// stores result in p1, a should be a value between 0 (p1 is transparent) and 255 (p1 is opaque)
inline void TRANPX32i(Uint32 &p1, Uint32 &p2, unsigned char &a, unsigned char &a2)
{
   Uint32 px;
   px = (((p1 & 0x00ff00ff) * a) >> 8) & 0xff00ff;
   px |= (((p1 & 0xff00ff00) >> 8) * a) & 0xff00ff00;
   p1 = px;
   px = (((p2 & 0x00ff00ff) * a2) >> 8) & 0xff00ff;
   px |= (((p2 & 0xff00ff00) >> 8) * a2) & 0xff00ff00;
   ADDPX32(p1, px);
}

// applies translucency, 8bit-8bit-8bit-8bit
// stores result in p1, a should be a value between 0 (p1 is transparent) and 255 (p1 is opaque)
inline void TRANPX24i(Uint32 &p1, Uint32 &p2, unsigned char &a, unsigned char &a2)
{
   Uint32 px;
   px = p1 & 0xff000000;
   px |= (((p1 & 0x00ff00ff) * a) >> 8) & 0x00ff00ff;
   px |= (((p1 & 0x0000ff00) >> 8) * a) & 0x0000ff00;
   p1 = px;
   px = p2 & 0xff000000;
   px |= (((p2 & 0x00ff00ff) * a2) >> 8) & 0x00ff00ff;
   px |= (((p2 & 0x0000ff00) >> 8) * a2) & 0x0000ff00;
   ADDPX24(p1, px);
}

// applies translucency, 5bit-6bit-5bit
// stores result in p1, a should be a value between 0 (p1 is transparent) and 63 (p1 is opaque)
inline void TRANPX16i(Uint16 &p1, Uint16 &p2, unsigned char &a, unsigned char &a2)
{
   Uint16 px;
   px = ((p1 & 0x1F) * a) >> 6;
   px |= (((p1 & 0x07E0) * a) >> 6) & 0x7E0;
   px |= (((p1 >> 11) * a) & 0x7C0) << 5;
   p1 = px;
   px = ((p2 & 0x1F) * a2) >> 6;
   px |= (((p2 & 0x07E0) * a2) >> 6) & 0x7E0;
   px |= (((p2 >> 11) * a2) & 0x7C0) << 5;
   ADDPX16(p1, px);
}



#else
void ADDPX32(Uint32 &d, Uint32 &s);
void ADDPX32old(Uint32 &d, Uint32 &s);
void ADDPX24(Uint32 &d, Uint32 &s);
void ADDPX16(Uint16 &d, Uint16 &s);
void ADDTOPX32(Uint32 &d, Uint32 &v);
void ADDTOPX24(Uint32 &d, Uint32 &v);
void ADDTOPX16(Uint16 &d, Uint16 &v);


void SUBPX32(Uint32 &d, Uint32 &s);
void SUBPX24(Uint32 &d, Uint32 &s);
void SUBPX16(Uint16 &d, Uint16 &s);
void SUBFROMPX32(Uint32 &d, Uint32 &v);
void SUBFROMPX24(Uint32 &d, Uint32 &v);
void SUBFROMPX16(Uint16 &d, Uint16 &v);

void MULPX32i(Uint32 &d, Uint32 &v);
void MULPX24i(Uint32 &d, Uint32 &v);
void MULPX16i(Uint16 &d, Uint16 &v);


void TRANPX32i(Uint32 &p1, Uint32 &p2, unsigned char &amount);
void TRANPX24i(Uint32 &p1, Uint32 &p2, unsigned char &amount);
void TRANPX16i(Uint16 &p1, Uint16 &p2, unsigned char &amount);

#endif

#endif
