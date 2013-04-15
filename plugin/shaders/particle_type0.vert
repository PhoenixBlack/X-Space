// Parameters
varying vec2 v_texCoord2D;
//varying vec4 v_vertex;
varying vec2 v_lightProjection;
varying float particleAge;
varying float particleSize;
varying float particleOffset;


//==============================================================================
void main(void)
{
  //Calculate particle size
  particleAge = gl_Color.a;
  particleSize = (128.0+particleAge*1400.0)*clamp(particleAge*64.0,0,1+2*particleAge*particleAge);
  particleOffset = gl_Color.b;
  
  //Compute vertex
  float x = -2*(gl_MultiTexCoord0.x-0.5);
  float y =  2*(gl_MultiTexCoord0.y-0.5);
  vec4 srcVertex = vec4(x*particleSize*0.5,y*particleSize*0.5,0,1);
  
  //Vectors
  vec3 look,up,right;

  //Get look vector
  look = vec3(gl_ModelViewMatrixInverse * vec4(0,0,0,1));
  look = normalize(look - gl_Vertex.xyz);
  
  //Get up vector
  up = vec3(gl_ModelViewMatrix[0][1],
            gl_ModelViewMatrix[1][1],
            gl_ModelViewMatrix[2][1]);

  //right = up x look
  right = cross(up,look);
  //up = look x right
  up = cross(look,right);

  //Billboarding matrix
  mat4 billboardMatrix = mat4(
    vec4(right,0),
    vec4(up,0),
    vec4(look,0),
    vec4(0,0,0,1));
  
  //Compute vertex position
  gl_Position = gl_ModelViewProjectionMatrix *
    vec4((billboardMatrix*srcVertex).xyz + gl_Vertex.xyz,1);
    
  //Compute vector to light
  vec3 lightVector = gl_LightSource[0].position.xyz;

  //Pass parameters to fragment shader
  v_texCoord2D = gl_MultiTexCoord0.xy;
//  v_vertex = gl_Position;
  v_lightProjection = vec2(dot(lightVector,right),dot(lightVector,up));
}
