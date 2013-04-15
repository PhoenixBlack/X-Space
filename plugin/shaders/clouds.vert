// Parameters
varying vec2 v_texCoord2D;
varying vec4 v_color;
varying vec4 v_src_color;
varying float v_distance;


//==============================================================================
void main(void)
{
  //Compute vertex position, coordinates
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
  v_texCoord2D = gl_MultiTexCoord0.xy;
  
  //Apply lighting
  vec3 vertex_normal = normalize(gl_NormalMatrix * gl_Normal);
  vec3 vertex_light_position = gl_LightSource[0].position.xyz;
  float diffuse = max(dot(vertex_normal, vertex_light_position), 0.0);
    
  //Get modulation color
  v_src_color = gl_Color;
  v_color = gl_Color * diffuse * diffuse;
  v_color[3] = v_src_color[3];
  
  //Get distance from camera for LOD
  v_distance = gl_Position[3];
}
