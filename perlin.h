#ifndef PROCEDURAL_TERRAIN_GENERATION_PERLIN_H
#define PROCEDURAL_TERRAIN_GENERATION_PERLIN_H

extern int seed;  // seed generated at the beginning of the execution, used to generate the same output given the same input

float fractal_noise(const float x, const float y, float freq, const int octaves);

#endif //PROCEDURAL_TERRAIN_GENERATION_PERLIN_H
