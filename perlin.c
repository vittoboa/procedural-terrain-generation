#include <cglm/cglm.h>
#include <math.h>

#include "perlin.h"

static const unsigned char permutations[] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,
    49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

// get gradient from integer coordinates
static inline unsigned char gradient(const int x, const int y)
{
    // determine the permutations index based on the y coordinate
    unsigned char index_y = (y + seed) % 256;
    if (index_y < 0) {
        index_y += 256;
    }

    // determine the permutations index based on the x coordinate
    unsigned char index_x = (permutations[index_y] + x) % 256;
    if (index_x < 0) {
        index_x += 256;
    }

    // get gradient
    return permutations[index_x];
}

// compute 2-dimensional perlin noise at coordinates x, y
static float perlin_noise(const float x, const float y)
{
    // determine point cell coordinates
    const int x_int = floor(x);
    const int y_int = floor(y);

    // get gradients from grid cell coordinates
    const unsigned char top_left     = gradient(x_int    , y_int);      // (0, 0)
    const unsigned char top_right    = gradient(x_int + 1, y_int);      // (1, 0)
    const unsigned char bottom_left  = gradient(x_int    , y_int + 1);  // (0 ,1)
    const unsigned char bottom_right = gradient(x_int + 1, y_int + 1);  // (1, 1)

    // determine interpolation weights
    const float x_dec = x - x_int;
    const float y_dec = y - y_int;

    // interpolate between grid point gradients
    const float top    = glm_smoothinterp(top_left, top_right, x_dec);
    const float bottom = glm_smoothinterp(bottom_left, bottom_right, x_dec);

    return glm_smoothinterp(top, bottom, y_dec);
}

// compute a fractal pattern as a sum of noise layers
float fractal_noise(const float x, const float y, float freq, const int octaves)
{
    const float lacunarity = 2.0;
    const float gain = 0.5;
    float fractal = 0.0;
    float amp = gain;

    // sum noise layers
    for (int i = 0; i < octaves; ++i) {
        fractal += perlin_noise(x * freq, y * freq) * amp;

        // increase the frequency for more details
        freq *= lacunarity;

        // diminish the influence of new details in the overall result
        amp *= gain;
    }

    return fractal / 256;
}
