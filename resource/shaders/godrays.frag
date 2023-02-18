#version 460 core

/*
from nvidia's samples
float4 main(float2 texCoord : TEXCOORD0) : COLOR0 
{   // Calculate vector from pixel to light source in screen space.    
    half2 deltaTexCoord = (texCoord - ScreenLightPos.xy);   
    // Divide by number of samples and scale by control factor.   
    deltaTexCoord *= 1.0f / NUM_SAMPLES * Density;   
    // Store initial sample.    
    half3 color = tex2D(frameSampler, texCoord);   
    // Set up illumination decay factor.    
    half illuminationDecay = 1.0f;   
    // Evaluate summation from Equation 3 NUM_SAMPLES iterations.    
    for (int i = 0; i < NUM_SAMPLES; i++)   
    {     // Step sample location along ray.     
        texCoord -= deltaTexCoord;     // Retrieve sample at new location.    
        half3 sample = tex2D(frameSampler, texCoord);     
        // Apply sample attenuation scale/decay factors.     
        sample *= illuminationDecay * Weight;     
        // Accumulate combined color.     
        color += sample;     
        // Update exponential decay factor.     
        illuminationDecay *= Decay;   
    }   
    // Output final color with a further scale control factor.    
    return float4( color * Exposure, 1); 
} 
*/

in vec2 texCoord;

layout(binding = 0)uniform sampler2D occlusionTexture;
layout(location = 0)uniform float density;
layout(location = 1)uniform float weight;
layout(location = 2)uniform float decay;
layout(location = 3)uniform float exposure;
layout(location = 4)uniform int numSamples;
layout(location = 5)uniform vec2 screenSpaceLightPos = vec2(0.0);

layout(location = 0)out vec4 FragColor;

void main(void) {
	vec3 fragColor = texture(occlusionTexture,texCoord).xyz;
	
	vec2 deltaTextCoord = vec2(texCoord - screenSpaceLightPos.xy);

	vec2 textCoo = texCoord.xy;
	deltaTextCoord *= (1.0 /  float(numSamples)) * density;
	float illuminationDecay = 1.0;

	for(int i=0; i < numSamples ; i++){
		textCoo -= deltaTextCoord;
		vec3 samp = texture(occlusionTexture, textCoo).xyz;
		samp *= illuminationDecay * weight;
		fragColor += samp;
		illuminationDecay *= decay;
	}
	fragColor *= exposure;
	FragColor = vec4(fragColor, 1.0);

}
