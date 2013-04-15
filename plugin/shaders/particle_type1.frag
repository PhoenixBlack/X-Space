//==============================================================================
// Reentry trail shader for X-Space
//
// Based on 2D perlin noise code by Stefan Gustavson
// (ITN-LiTH (stegu@itn.liu.se) 2004-12-05)
//==============================================================================

// Uniforms
uniform sampler2D permTexture;

// Parameters
varying vec2 v_texCoord2D;
varying float particleAge;
varying float particleSize;
varying float particleOffset;
varying float particleParameter;

// Constants for (1/256) and (0.5/256) for a 256x256 perm texture
#define ONE 0.00390625
#define ONEHALF 0.001953125

// 5th degree smooth interpolation function for Perlin "improved noise".
float fade(const in float t) {
  //return t*t*(3.0-2.0*t); //Yields discontinuous second derivative
  return t*t*t*(t*(t*6.0-15.0)+10.0); //Yields C2-continuous noise
}

//2D perlin noise
float noise2d(const in vec2 P)
{
  vec2 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled and offset for texture lookup
  vec2 Pf = fract(P);             // Fractional part for interpolation

  // Noise contribution from lower left corner
  vec2 grad00 = texture2D(permTexture, Pi).rg * 4.0 - 1.0;
  float n00 = dot(grad00, Pf);

  // Noise contribution from lower right corner
  vec2 grad10 = texture2D(permTexture, Pi + vec2(ONE, 0.0)).rg * 4.0 - 1.0;
  float n10 = dot(grad10, Pf - vec2(1.0, 0.0));

  // Noise contribution from upper left corner
  vec2 grad01 = texture2D(permTexture, Pi + vec2(0.0, ONE)).rg * 4.0 - 1.0;
  float n01 = dot(grad01, Pf - vec2(0.0, 1.0));

  // Noise contribution from upper right corner
  vec2 grad11 = texture2D(permTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
  float n11 = dot(grad11, Pf - vec2(1.0, 1.0));

  // Blend contributions along x
  vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade(Pf.x));

  // Blend contributions along y
  float n_xy = mix(n_x.x, n_x.y, fade(Pf.y));

  // We're done, return the final noise value.
  return n_xy;
}




//==============================================================================
//Normalization coefficient
#define NORM 1e-6
//Planck's constant
#define H 6.26e-34
//Speed of light
#define C 299792458

//Red wavelength
#define R_WL 700e-9
//Green wavelength
#define G_WL 540e-9
//Blue wavelength
#define B_WL 450e-9

void main(void)
{
  vec2 noise_offset = particleOffset*1024.0*vec2(1,1);
  
  float noise = 0.0;
  noise += noise2d((v_texCoord2D+noise_offset)* 2.0)*1.0;
  noise += noise2d((v_texCoord2D+noise_offset)* 4.0)*0.8;
  noise += noise2d((v_texCoord2D+noise_offset)* 8.0)*0.8;

  float r = 2*sqrt((v_texCoord2D.x-0.5)*(v_texCoord2D.x-0.5)+
                   (v_texCoord2D.y-0.5)*(v_texCoord2D.y-0.5));
  float shape = 1-max(0,1.0-r);
  float density = max(0,(0.5+0.5*noise)-particleAge-pow(shape,8));
  
  float t = particleParameter*sqrt(1-shape);
  vec3 pl_c = vec3(
    min(1,pow(t/700,2)),
    min(1,pow(t/1200,2)),
    min(1,pow(t/2200,2)));
  pl_c *= density;
    
  float alpha = (0.2-0.2*particleAge+(1-shape))*clamp((t-700)/400,0,1);
  
  alpha = 0.0;
  gl_FragColor = vec4(pl_c.x,pl_c.y,pl_c.z,alpha);
}
