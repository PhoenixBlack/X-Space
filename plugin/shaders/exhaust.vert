// Parameters
varying vec2 v_texCoord2D;
varying vec4 v_color;
varying vec4 v_vertex;

// Uniforms
uniform float trailWidth;
uniform float trailHeight;


//==============================================================================
void main(void)
{
  float x = -2*(gl_MultiTexCoord0.x-0.5);
  float y =     gl_MultiTexCoord0.y;
  vec4 srcVertex = vec4(2*x*trailWidth,2*y*trailHeight,0,1)+vec4(gl_Vertex.xyz,0);
  
  //Get look vector
  vec3 look = vec3(gl_ModelViewMatrixInverse * vec4(0,0,0,1));
  look = normalize(look - gl_Vertex.xyz);
  
  //X axis is the top one
  look.x = 1.0;
  look = normalize(look);
  vec3 up = vec3(1.0,0.0,0.0);
  
  //right = up x look
  vec3 right = normalize(cross(up,look));
  
  mat4 billboardMatrix = mat4(
    vec4(right,0),
    vec4(up,0),
    vec4(look,0),
    vec4(0,0,0,1));
  
  //Compute vertex position
  gl_Position = gl_ModelViewProjectionMatrix * billboardMatrix * srcVertex;

  //Pass parameters to fragment shader
  v_texCoord2D = gl_MultiTexCoord0.xy;
  v_color = gl_Color;
  v_vertex = gl_Position;
}
