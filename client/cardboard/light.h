#ifndef LIGHTS_H
#define LIGHTS_H


// -- Fog and Light -- 
// These are stored in a single struct for easier handling.
typedef struct light_s
{
   // -- Light --
   // The color multipliers work on fixed point scheme of sorts. The multipliers
   // are stored as 1.8 fixed point numbers (max should always be 256)
   // the actual pixel values are multiplied by this integer and then shifted back down
   // discarding the newly formed fractional bits and effectivly scaling the pixel value.

   // l_level controls the distance fading calculations and l_r, l_g, and l_b are the 
   // individual color multipliers. These values are multiplied by whatever brightness value
   // is calculated for a wall column or floor span and are then used as multipliers on the colors. 
   Uint16 l_level;
   Uint16 l_r, l_g, l_b;

   // -- Fog --
   // The fog system works much like the lighting system except the lighting system fades the colors
   // gradually to 0, the fog system fades colors gradually to another single color. The system
   // works on almost the exact same concept (pixel color values are multiplied by fixed point
   // multipliers and shifted) but additional color is then added (is effectivly 1 - scale)
   // thus facilitating the fade to a color.
   float f_start, f_stop;
   Uint8 f_r, f_g, f_b;
} light_t;

#endif
