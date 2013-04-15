//==============================================================================
// Re-entry plasma shader for X-Space
//
// Based on 2D perlin noise code by Stefan Gustavson
// (ITN-LiTH (stegu@itn.liu.se) 2004-12-05)
//==============================================================================

// Uniforms
uniform sampler2D permTexture;
uniform float curTime;
uniform float machNo;
uniform float totalArea;
uniform float heatFlux;

// Parameters
varying vec3 v_vertex;
varying vec3 v_velocity;
varying float v_z;

// Constants for (1/256) and (0.5/256) for a 256x256 perm texture
#define ONE 0.00390625
#define ONEHALF 0.001953125

// 5th degree smooth interpolation function for Perlin "improved noise".
float fade(const in float t) {
  //return t*t*(3.0-2.0*t); //Yields discontinuous second derivative
  return t*t*t*(t*(t*6.0-15.0)+10.0); //Yields C2-continuous noise
}

//3D perlin noise
float noise3d(const in vec3 P)
{
  vec3 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled so +1 moves one texel
                                  // and offset 1/2 texel to sample texel centers
  vec3 Pf = fract(P);     // Fractional part for interpolation

  // Noise contributions from (x=0, y=0), z=0 and z=1
  float perm00 = texture2D(permTexture, Pi.xy).a ;
  vec3  grad000 = texture2D(permTexture, vec2(perm00, Pi.z)).rgb * 4.0 - 1.0;
  float n000 = dot(grad000, Pf);
  vec3  grad001 = texture2D(permTexture, vec2(perm00, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n001 = dot(grad001, Pf - vec3(0.0, 0.0, 1.0));

  // Noise contributions from (x=0, y=1), z=0 and z=1
  float perm01 = texture2D(permTexture, Pi.xy + vec2(0.0, ONE)).a ;
  vec3  grad010 = texture2D(permTexture, vec2(perm01, Pi.z)).rgb * 4.0 - 1.0;
  float n010 = dot(grad010, Pf - vec3(0.0, 1.0, 0.0));
  vec3  grad011 = texture2D(permTexture, vec2(perm01, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n011 = dot(grad011, Pf - vec3(0.0, 1.0, 1.0));

  // Noise contributions from (x=1, y=0), z=0 and z=1
  float perm10 = texture2D(permTexture, Pi.xy + vec2(ONE, 0.0)).a ;
  vec3  grad100 = texture2D(permTexture, vec2(perm10, Pi.z)).rgb * 4.0 - 1.0;
  float n100 = dot(grad100, Pf - vec3(1.0, 0.0, 0.0));
  vec3  grad101 = texture2D(permTexture, vec2(perm10, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n101 = dot(grad101, Pf - vec3(1.0, 0.0, 1.0));

  // Noise contributions from (x=1, y=1), z=0 and z=1
  float perm11 = texture2D(permTexture, Pi.xy + vec2(ONE, ONE)).a ;
  vec3  grad110 = texture2D(permTexture, vec2(perm11, Pi.z)).rgb * 4.0 - 1.0;
  float n110 = dot(grad110, Pf - vec3(1.0, 1.0, 0.0));
  vec3  grad111 = texture2D(permTexture, vec2(perm11, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n111 = dot(grad111, Pf - vec3(1.0, 1.0, 1.0));

  // Blend contributions along x
  vec4 n_x = mix(vec4(n000, n001, n010, n011),
                 vec4(n100, n101, n110, n111), fade(Pf.x));

  // Blend contributions along y
  vec2 n_xy = mix(n_x.xy, n_x.zw, fade(Pf.y));

  // Blend contributions along z
  float n_xyz = mix(n_xy.x, n_xy.y, fade(Pf.z));

  // We're done, return the final noise value.
  return n_xyz;
}




//==============================================================================
void main(void)
{
  vec3 s_vertex = v_vertex*(0.3-0.15*length(v_vertex)/100);
  //*(vec3(1,1,1) + 0.4*v_velocity);
//  s_vertex.x = 1.0-0.8*v_velocity.x;
  vec3 offset = s_vertex-v_velocity*curTime*50.0;

  float noise = 0.0;
  noise += noise3d(offset*0.5);
  noise += noise3d(offset*1.0)*0.5;
//  noise += noise3d(offset*2.0)*0.25;
  
  float density_norm = -(1-machNo/30)-clamp(1-heatFlux*0.06,0,1);
  float density = clamp(0.5+0.5*noise+density_norm,0,1);
  
  float tau = 4/(100*totalArea);
  float f = exp(-tau*length(v_vertex));
  float alpha = 0.5*f*clamp(v_z*0.05,0,1);
  float T = max(0,250*density-50*(1-f));

  vec3 color = vec3(
    1e-1/((exp(0.0136/(4.5*T))-1)*pow(4.5,5)),
    1e-1/((exp(0.0136/(5.4*T))-1)*pow(5.4,5)),
    1e-1/((exp(0.0136/(7.0*T))-1)*pow(7.0,5)));
  
  gl_FragColor = vec4(color,alpha);
}
