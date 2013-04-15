//==============================================================================
// Reentry plasma glow postprocessing shader for X-Space
//==============================================================================

// Uniforms
uniform sampler2D frameBuffer;
uniform sampler2D zBuffer;

// Parameters
varying vec2 texCoord;

const float blurSize = 1.0/256.0;


//==============================================================================
void main(void)
{
  vec4 result = vec4(0,0,0,0);

  result += texture2D(frameBuffer, vec2(texCoord.x -  4.0*blurSize, texCoord.y)) * 0.05;
  result += texture2D(frameBuffer, vec2(texCoord.x -  3.0*blurSize, texCoord.y)) * 0.09;
  result += texture2D(frameBuffer, vec2(texCoord.x -  2.0*blurSize, texCoord.y)) * 0.12;
  result += texture2D(frameBuffer, vec2(texCoord.x -  1.0*blurSize, texCoord.y)) * 0.15;
  result += texture2D(frameBuffer, vec2(texCoord.x +  0.0*blurSize, texCoord.y)) * 0.16;
  result += texture2D(frameBuffer, vec2(texCoord.x +  1.0*blurSize, texCoord.y)) * 0.15;
  result += texture2D(frameBuffer, vec2(texCoord.x +  2.0*blurSize, texCoord.y)) * 0.12;
  result += texture2D(frameBuffer, vec2(texCoord.x +  3.0*blurSize, texCoord.y)) * 0.09;
  result += texture2D(frameBuffer, vec2(texCoord.x +  4.0*blurSize, texCoord.y)) * 0.05;
  
  result += texture2D(frameBuffer, vec2(texCoord.x, texCoord.y - 4.0*blurSize)) * 0.05;
  result += texture2D(frameBuffer, vec2(texCoord.x, texCoord.y - 3.0*blurSize)) * 0.09;
  result += texture2D(frameBuffer, vec2(texCoord.x, texCoord.y - 2.0*blurSize)) * 0.12;
  result += texture2D(frameBuffer, vec2(texCoord.x, texCoord.y - 1.0*blurSize)) * 0.15;
  result += texture2D(frameBuffer, vec2(texCoord.x, texCoord.y + 0.0*blurSize)) * 0.16;
  result += texture2D(frameBuffer, vec2(texCoord.x, texCoord.y + 1.0*blurSize)) * 0.15;
  result += texture2D(frameBuffer, vec2(texCoord.x, texCoord.y + 2.0*blurSize)) * 0.12;
  result += texture2D(frameBuffer, vec2(texCoord.x, texCoord.y + 3.0*blurSize)) * 0.09;
  result += texture2D(frameBuffer, vec2(texCoord.x, texCoord.y + 4.0*blurSize)) * 0.05;
  
  result.a = 1.0;
  gl_FragColor = result;
}

/*float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);
float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);

layout(location = FRAG_COLOR) out float color;

void main(void)
{
    vec2 size = vec2(textureSize(material.diffuseTexture, 0));

    color = texture(material.diffuseTexture, vertex.texcoord).r * weight[0];

    for (int i = 1; i < 3; ++i)
    {
        color += texture(material.diffuseTexture, vertex.texcoord + vec2(offset[i], 0.0) / size.x).r * weight[i];
        color += texture(material.diffuseTexture, vertex.texcoord - vec2(offset[i], 0.0) / size.x).r * weight[i];
    }
}*/
