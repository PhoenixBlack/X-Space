//
// Atmospheric scattering vertex shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

uniform vec3 v3CameraPos;     //The camera's current position
uniform vec3 v3LightPos;      //The direction vector to the light source
uniform float fCameraHeight;  //The camera's current height
uniform float fCameraHeight2; //fCameraHeight^2
varying vec3 v3Direction;     //Direction to fragment

//The outer (atmosphere) radius
const float fOuterRadius = 6378000.0f+100000.0f;
//The inner (planetary) radius
const float fInnerRadius = 6378000.0f;
//Atmosphere cutoff (for X-Plane)
const float fCutoffRadius = 6378000.0f+15000.0f;
//The scale depth (i.e. the altitude at which the atmosphere's average density is found)
const float fScaleDepth = 0.25f;
//Number of samples
const int nSamples = 1;

//Rayleigh scattering constant
const float Kr = 0.0025f;
//Mie scattering constant
const float Km = 0.0015f;
//Sun brightness constant
const float ESun = 15.0f;
//The Mie phase asymmetry factor
const float g = -0.95f;
//Inverse wavelength
const vec3 v3InvWavelength = vec3(
  1.0f/pow(0.650f,4.0f), // 650 nm for red
  1.0f/pow(0.570f,4.0f), // 570 nm for green
  1.0f/pow(0.475f,4.0f)  // 475 nm for blue
);

//Other constants
const float fOuterRadius2 = fOuterRadius*fOuterRadius;
const float fInnerRadius2 = fInnerRadius*fInnerRadius;
const float fKrESun = Kr * ESun;
const float fKmESun = Km * ESun;
const float fKr4PI = Kr * 4 * 3.1415926;
const float fKm4PI = Km * 4 * 3.1415926;
const float fScale = 1 / (fOuterRadius - fInnerRadius);
const float fScaleOverScaleDepth = fScale / fScaleDepth;
const float fSamples = nSamples;

float scale(float fCos)
{
  float x = 1.0 - fCos;
  return fScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

void main(void)
{
  // Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere)
  vec3 v3Pos = gl_Vertex.xyz;
  vec3 v3Ray = v3Pos - v3CameraPos;
  float fFar = length(v3Ray);
  v3Ray /= fFar;

  // Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)
  float B = 2.0 * dot(v3CameraPos, v3Ray);
  float C = fCameraHeight2 - fOuterRadius2;
  float fDet = max(0.0, B*B - 4.0 * C);
  float fNear = 0.5 * (-B - sqrt(fDet));
  fNear = fNear*clamp(floor(C),0,1);

  // Calculate the ray's starting position, then calculate its scattering offset
  vec3 v3Start = v3CameraPos + v3Ray * fNear;
  fFar -= fNear;
  float fDepth = exp((fInnerRadius - fOuterRadius) / fScaleDepth);
  float fCameraAngle = dot(-v3Ray, v3Pos) / length(v3Pos);
  float fLightAngle = dot(v3LightPos, v3Pos) / length(v3Pos);
  float fCameraScale = scale(fCameraAngle);
  float fLightScale = scale(fLightAngle);
  float fCameraOffset = fDepth*fCameraScale;
  float fTemp = (fLightScale + fCameraScale);

  // Initialize the scattering loop variables
  float fSampleLength = fFar / fSamples;
  float fScaledLength = fSampleLength * fScale;
  vec3 v3SampleRay = v3Ray * fSampleLength;
  vec3 v3SamplePoint = v3Start + v3SampleRay * 0.5;

  // Now loop through the sample rays
  vec3 v3FrontColor = vec3(0.0, 0.0, 0.0);
  vec3 v3Attenuate;
  for(int i=0; i<nSamples; i++)
  {
    float fHeight = length(v3SamplePoint);
    float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
    float fScatter = fDepth*fTemp - fCameraOffset;
    v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
    v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
    v3SamplePoint += v3SampleRay;
  }

  // Calculate the attenuation factor for the ground
  gl_FrontSecondaryColor.rgb = v3Attenuate;
  gl_FrontColor.rgb = v3FrontColor * (v3InvWavelength * fKrESun + fKmESun);
  gl_FrontColor.a = clamp((fCameraHeight-fCutoffRadius)*(1/(fOuterRadius-fCutoffRadius)),0,1);
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
