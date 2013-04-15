//==============================================================================
// Exhaust smoke shader for X-Space
//
// Based on 2D perlin noise code by Stefan Gustavson
// (ITN-LiTH (stegu@itn.liu.se) 2004-12-05)
//==============================================================================

// Uniforms
uniform sampler2D permTexture;

// Parameters
varying vec2 v_texCoord2D;
//varying vec4 v_vertex;
varying vec2 v_lightProjection;
varying float particleAge;
varying float particleSize;
varying float particleOffset;

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
void main(void)
{
  vec2 noise_offset = particleOffset*256.0*vec2(1,1);
  
  float noise = 0.0;
  noise += noise2d((v_texCoord2D+noise_offset)*2.0)*1.0;
  noise += noise2d((v_texCoord2D+noise_offset)*4.0)*0.6;
  noise += noise2d((v_texCoord2D+noise_offset)*8.0)*0.4;

  float r = 2*sqrt((v_texCoord2D.x-0.5)*(v_texCoord2D.x-0.5)+
                   (v_texCoord2D.y-0.5)*(v_texCoord2D.y-0.5));
  float shape = 1-max(0,1.0-r);
  
  float density = max(0,(0.5+0.5*noise)-particleAge-pow(shape,2));
  float alpha = density*clamp(particleAge*32.0,0,1);
  
  vec2 lightAxis = vec2(v_lightProjection.y,v_lightProjection.x)*(2.0-alpha);
  float c = dot(lightAxis,vec2(-0.5,-0.5));
  float ab = dot(lightAxis,v_texCoord2D);
  float shadowGradient = ab+c;
  float shadow = clamp(0.7+2.0*shadowGradient,0.2,1.0);
  
  gl_FragColor = vec4(shadow,shadow,shadow,alpha);
}
