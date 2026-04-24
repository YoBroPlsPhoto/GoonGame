#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragPosition;
in vec3 fragNormal;
in vec4 shadowCoord;
in float fragDepth;
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec4 lightColor;
uniform sampler2D reflectionTexture;
uniform sampler2D shadowMap;
uniform mat4 matView;
out vec4 finalColor;

float shadowCalculation(vec4 sc)
{
    vec3 projCoords = sc.xyz / sc.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = 0.005;
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 lightDir = normalize(lightPos - fragPosition);
    float shadow = shadowCalculation(shadowCoord);
    
    // ALL VERTICAL SURFACES = GLASS
    bool isVertical = abs(normal.y) < 0.5;
    
    // Thin steel frame grid on glass
    vec2 windowUV;
    if (abs(normal.x) > 0.5) windowUV = fragPosition.zy;
    else if (abs(normal.z) > 0.5) windowUV = fragPosition.xy;
    else windowUV = fragPosition.xz;
    
    float frameLineH = 0.4;
    float frameLineV = 0.4;
    float floorH = 4.0;
    float panelW = 5.0;
    
    bool isFrame = false;
    if (isVertical && (fragColor.a > 0.999)) {
        float gy = mod(windowUV.y, floorH);
        float gx = mod(windowUV.x, panelW);
        if (gy < 0.0) gy += floorH;
        if (gx < 0.0) gx += panelW;
        isFrame = (gy < frameLineH) || (gx < frameLineV);
    }
    
    bool isWindow = isVertical && !isFrame && (fragPosition.y > 5.0) && (fragColor.a > 0.999);

    vec3 ambient = vec3(0.2, 0.22, 0.28) * fragColor.rgb;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = (1.0 - shadow) * diff * lightColor.rgb * fragColor.rgb;
    
    vec3 combined;

    if (isWindow) {
        // Bright reflective glass SKYSCRAPERS
        vec3 glassBase = vec3(0.25, 0.35, 0.55);
        vec3 skyColor = vec3(0.5, 0.7, 0.9);
        
        vec3 refl = reflect(-viewDir, normal);
        
        // --- STABLE WORLD-SPACE MAPPING ---
        // Project world-space reflection vector to 2D coords
        // This is much more stable than view-space projection
        vec2 reflCoord = vec2(refl.x * 0.5 + 0.5, refl.z * 0.5 + 0.5);
        vec3 mirrorColor = texture(reflectionTexture, reflCoord).rgb;
        
        mirrorColor = max(mirrorColor, skyColor * 0.3);
        float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);
        float reflectStrength = mix(0.5, 0.8, fresnel);

        combined = mix(glassBase, mirrorColor, reflectStrength * 0.4); // Very subtle
        
        // --- SUN SPECULAR REFLECTION (PROMINENT & STABLE) ---
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), 512.0);
        combined += spec * lightColor.rgb * 8.0 * (1.0 - shadow);
        
        combined += (1.0 - shadow) * diff * 0.15;
        
        finalColor = vec4(combined, 1.0);
    } else if (isFrame && isVertical) {
        // Steel frame lines
        combined = vec3(0.25, 0.25, 0.28) + (diffuse * 0.2);
        finalColor = vec4(combined, 1.0);
    } else {
        // --- 100% UNIFORM MATTE (Matches unshaded mirror pass) ---
        finalColor = fragColor;
    }

    // --- LINEAR FOG CALCULATION (STABLE) ---
    float fogStart = 500.0;
    float fogEnd = 3500.0;
    float fogFactor = (fragDepth - fogStart) / (fogEnd - fogStart);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    vec3 skyFogColor = vec3(0.4, 0.75, 1.0); // SKYBLUE
    finalColor.rgb = mix(finalColor.rgb, skyFogColor, fogFactor);
}
