// Parameters
varying vec2 v_texCoord2D;
varying float particleAge;
varying float particleSize;
varying float particleOffset;
varying float particleParameter;


//==============================================================================
void main(void)
{
  //Calculate particle size
  particleAge = gl_Color.a; //
  particleSize = 512.0*(1.1-exp(-particleAge*32))*exp(-particleAge*8);
  particleOffset = gl_Color.b;
  particleParameter = gl_Color.g*2000.0-particleAge*1500;
  
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

  //Pass parameters to fragment shader
  v_texCoord2D = gl_MultiTexCoord0.xy;
}
