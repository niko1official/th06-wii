uniform bool useTexCoords;

uniform sampler2D tex;
uniform float fogNear;
uniform float fogFar;
uniform vec4 fogColor;
uniform int colorOp;
uniform vec4 envDiffuse;

varying vec2 interpTexCoords;
varying vec4 interpDiffuse;
varying float viewZ;

const float alphaThreshold = 4.0 / 255.0;

#define OP_MODULATE 0
#define OP_ADD 1
#define OP_REPLACE 2

void main()
{
    vec4 fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 fragArg1 = fragColor;
    vec4 fragArg2 = fragColor;

    if (useTexCoords)
    {
        fragArg1 = texture2D(tex, interpTexCoords);
    }
    else
    {
        fragArg1 = interpDiffuse;
    }

#ifndef NO_VERTEX_BUFFER
    fragArg2 = envDiffuse;
#else
    fragArg2 = interpDiffuse;
#endif

    if (colorOp == OP_MODULATE)
    {
        fragColor = fragArg1 * fragArg2;
    }
    else if (colorOp == OP_ADD)
    {

        fragColor.rgb = min(fragArg1.rgb + fragArg2.rgb, vec3(1.0, 1.0, 1.0));
        fragColor.a = fragArg1.a * fragArg2.a;
    }
    else if(colorOp == OP_REPLACE)
    {
        fragColor = fragArg1;
    }

#ifndef NO_FOG
    float fogCoefficient = (fogFar - viewZ) / (fogFar - fogNear);
    fogCoefficient = clamp(fogCoefficient, 0.0, 1.0);

    fragColor.rgb = mix(fogColor.rgb, fragColor.rgb, fogCoefficient);
#endif

#ifndef USE_FRAG_DEPTH
    if (fragColor.a < alphaThreshold)
    {
        discard;
    }
#else
    if (fragColor.a < alphaThreshold)
    {
        fragColor.a = 0.0;
        gl_FragDepthEXT = 1.0;
    }
#endif

    gl_FragColor = fragColor;
}
