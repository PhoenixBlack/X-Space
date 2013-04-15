// Parameters
varying vec3 v_vertex;
varying vec3 v_velocity;
varying float v_z;


//==============================================================================
void main(void)
{
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
  
  v_vertex    = gl_Vertex.xyz;
  v_velocity  = gl_Normal.xyz;
  v_z = gl_Position.w;
}
