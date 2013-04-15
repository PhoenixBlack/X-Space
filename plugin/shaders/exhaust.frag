//==============================================================================
// Rocket exhaust shader for X-Space
//
// Based on 2D perlin noise code by Stefan Gustavson
// (ITN-LiTH (stegu@itn.liu.se) 2004-12-05)
//==============================================================================

// Uniforms
uniform sampler2D permTexture;
uniform float curTime;
uniform float exhaustIntensity;
uniform float exhaustVelocity;
uniform float exhaustWidth;

// Parameters
varying vec2 v_texCoord2D;
varying vec4 v_color;
varying vec4 v_vertex;

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
  //True coordinates in the texture
  vec2 noise_coord = vec2(v_texCoord2D.x*1.0,v_texCoord2D.y*0.9-curTime*3.0);
  
  //Rocket exhaust plume shape
  float x = v_texCoord2D.y;
  float y = abs(v_texCoord2D.x-0.5)*2;
  float t = 0.5 + 0.5*pow(x,2);
  float r = sqrt(x*x*t+y*y);
  
  float shape_ys = 0.1+0.9*(1-exp(-x*1.5/exhaustVelocity));
  float shape = clamp((1.0*shape_ys-pow(r,1))*2.0,0,1);
  
  float core_l = 0.5*exhaustIntensity;
  float core_xs = clamp(core_l-x,0,1)*(1/core_l);
  float core_ys = clamp(1.0-2*x,0,1);
  float core = clamp(1-12*max(abs(y)-exhaustWidth*core_ys,0),0,1)*core_xs;

  //Accumulate noise
  float noise = 0.0;
  noise += noise2d(noise_coord *  1.00)*1.0;
  noise += noise2d(noise_coord *  3.00)*1.0;
  noise += noise2d(noise_coord *  4.00)*1.0;
  noise += noise2d(noise_coord *  8.00)*0.5;
  noise += noise2d(noise_coord * 12.00)*0.5;
  noise += noise2d(noise_coord * 20.00)*0.2;

  //Final brightness
  float brightness = clamp(1*(core+(1+noise-2*pow(x,1)*(1-1.5*shape))*shape),0,1);

  //Output color
  gl_FragColor = v_color * vec4(
    brightness*1.0,
    brightness*1.0*(1.0-0.5*y),
    brightness*(1.0-x),
    1.0
  );
}
