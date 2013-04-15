//
// Atmospheric scattering fragment shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

void main (void)
{
  gl_FragColor = gl_Color + 0.15 * gl_SecondaryColor;
  gl_FragColor.r = pow(gl_FragColor.r,2);
  gl_FragColor.g = pow(gl_FragColor.g,2);
  gl_FragColor.b = pow(gl_FragColor.b,2);
  gl_FragColor.rgb *= gl_Color.a;
}
