//==============================================================================
// Clouds layer shader for X-Space
//
// Based on 2D perlin noise code by Stefan Gustavson
// (ITN-LiTH (stegu@itn.liu.se) 2004-12-05)
//==============================================================================

// Uniforms
uniform sampler2D permTexture;
uniform sampler2D cloudTexture;

// Parameters
varying vec2 v_texCoord2D;
varying vec4 v_color;
varying vec4 v_src_color;
varying float v_distance;

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
  float level = texture2D(cloudTexture, v_texCoord2D).x;

  float noise_lod0 = max(0,1.0 - min(1,v_distance*1e-7));
  float noise_lod1 = max(0,1.0 - min(1,v_distance*0.3*1e-7));

  float noise = 0.0;
  noise += noise2d(v_texCoord2D *   256.0)*1.0;
  noise += noise2d(v_texCoord2D *   512.0)*0.9;
  noise += noise2d(v_texCoord2D *  1024.0)*0.8; //Low
  noise += noise2d(v_texCoord2D *  2048.0)*0.7*noise_lod0;
#ifdef QUALITY1
  noise += noise2d(v_texCoord2D *  4096.0)*0.6*noise_lod0; //Normal
  noise += noise2d(v_texCoord2D *  6000.0)*0.5*noise_lod1*(0.1+0.9*max(0,level-0.2)*(1/0.8));//*level;
  noise += noise2d(v_texCoord2D * 10000.0)*0.4*noise_lod1*(0.1+0.9*max(0,level-0.2)*(1/0.8));//*level; //High
  noise += noise2d(v_texCoord2D * 15000.0)*0.3*noise_lod1*(0.1+0.9*max(0,level-0.2)*(1/0.8));//*level; //High
#endif

  //Noise part of the cloud
  float cloud_noise = 0.8*1.35*level + 1.2*noise*(1-level)*level;
  //Constant part of the cloud
  float cloud_const = level*(1-0.5*level*level);
  //Total cloud layer
  float cloud = max(0,cloud_noise)*0.5+0.9*cloud_const;

  //Transparency of cloud (special case for shadows)
  float visibility = cloud*cloud*cloud; //*v_src_color.x + cloud*(1-v_src_color.x);
  gl_FragColor = v_color * vec4(cloud, cloud, cloud, visibility);
}
