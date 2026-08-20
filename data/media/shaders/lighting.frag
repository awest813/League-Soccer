#version 150

#pragma optimize(on)

uniform sampler2D map_albedo; // 0
uniform sampler2D map_normal; // 1
uniform sampler2D map_depth;  // was: 3, now: 2
//uniform sampler2D map_aux;    // was: 2, now: 3 (disabled though)
uniform sampler2DShadow map_shadow; // was: 7, now: 3

//uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 inverseProjectionViewMatrix;
uniform mat4 lightViewProjectionMatrix;

uniform float contextWidth;
uniform float contextHeight;
uniform float contextX;
uniform float contextY;

//uniform vec2 cameraClip;

uniform bool has_shadow;

uniform vec3 cameraPosition;
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float lightRadius;

out vec4 stdout0;
out vec4 stdout1;

vec3 GetWorldPosition(vec2 texCoord, float depth) {
  vec4 projectedPos = vec4(texCoord.x * 2 - 1, texCoord.y * 2 - 1, depth * 2 - 1, 1.0f);
  vec4 worldPosition = inverseProjectionViewMatrix * projectedPos;
  worldPosition.xyz /= worldPosition.w;
  return worldPosition.xyz;
}

void main(void) {

  vec2 texCoord = gl_FragCoord.xy;
  texCoord.x -= contextX;
  texCoord.y -= contextY;
  texCoord.x /= contextWidth;
  texCoord.y /= contextHeight;

  float depth = texture2D(map_depth, texCoord).x; // non-linear (0 .. 1)

  vec3 worldPosition = GetWorldPosition(texCoord, depth);

  // convert from non-linear to linear
  //float fragDepth = cameraClip.y / (depth - cameraClip.x);

  float dist = distance(lightPosition, worldPosition.xyz);
  // discard is sometimes slower than not discarding. so don't do this. if (dist > radius) discard;

  vec3 lightDir = normalize(lightPosition - worldPosition.xyz);

  vec4 normalshine = texture2D(map_normal, texCoord);
  vec3 normal = normalize(normalshine.xyz);

  float nDotLD = dot(lightDir, normal);
  //if (nDotLD < 0.0f) discard;

  // scattering emulation ;)
  float scatter_nDotLD = nDotLD * 0.5f + 0.5f;
  scatter_nDotLD = pow(scatter_nDotLD, 1.7f); // more 'normalish' curve on < 90 deg, but keep some for nuance on the back of objects
  nDotLD = max(nDotLD, 0.0f);

  // combine
  float resulting_nDotLD = nDotLD * 0.6f + scatter_nDotLD * 0.4f;

  // photoshop's 'curves' in a sinewave fashion
  //nDotLD = nDotLD * 0.2 + (-cos(nDotLD * 3.14159265) * 0.5 + 0.5) * 0.8;

  float shininess = normalshine.w;

  // linear
  float falloff = max(0.0, lightRadius - dist) / lightRadius;
  // exp
  falloff = falloff * falloff;
  // (above calculation is highly incorrect but gives a nice mix between exp and linear. see
  // http://tomdalling.com/blog/modern-opengl/07-more-lighting-ambient-specular-attenuation-gamma/ for more info about the correct methods
  // (basically: "1.0 / (1.0 + x ^ 2)" )

  float brightness = 2.0f;//1.5f;
  vec3 lighted = resulting_nDotLD * falloff * lightColor * brightness;

  // should be camerapos, why isn't it? vec3(inverse(viewMatrix)[3])

  vec3 viewDir = normalize(cameraPosition - worldPosition.xyz);
  vec3 halfDir = normalize(lightDir + viewDir);

  vec4 base = texture2D(map_albedo, texCoord);

  float spec = base.w;
  vec3 specularColor = lightColor;
  
  // Energy-conserving Blinn-Phong
  float shininessFactor = shininess * 128.0f + 1.0f; // prevent div by zero
  float energyConservation = (shininessFactor + 8.0f) / (8.0f * 3.14159265f);
  vec3 specular = pow(max(0.0f, dot(normal, halfDir)), shininessFactor) * spec * falloff * specularColor * energyConservation;

  float shaded = 1.0;
  if (has_shadow) {

    const int poisson_size = 16;
    vec2 poisson[poisson_size];
    poisson[0] = vec2( -0.94201624, -0.39906216 );
    poisson[1] = vec2( 0.94558609, -0.76890725 );
    poisson[2] = vec2( -0.094184101, -0.92938870 );
    poisson[3] = vec2( 0.34495938, 0.29387760 );
    poisson[4] = vec2( -0.91588581, 0.45771432 );
    poisson[5] = vec2( -0.81544232, -0.87912464 );
    poisson[6] = vec2( -0.38277543, 0.27676845 );
    poisson[7] = vec2( 0.97484398, 0.75648379 );
    poisson[8] = vec2( 0.44323325, -0.97511554 );
    poisson[9] = vec2( 0.53742981, -0.47373420 );
    poisson[10] = vec2( -0.26496911, -0.41893023 );
    poisson[11] = vec2( 0.79197514, 0.19090188 );
    poisson[12] = vec2( -0.24188840, 0.99706507 );
    poisson[13] = vec2( -0.81409955, 0.91437590 );
    poisson[14] = vec2( 0.19984126, 0.78641367 );
    poisson[15] = vec2( 0.14383161, -0.14100790 );

    // pseudo-random rotation
    float random = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    float s = sin(random * 6.2831853);
    float c = cos(random * 6.2831853);
    mat2 rot = mat2(c, -s, s, c);

    float offset = -0.0002f;
    vec4 projectedFrag = lightViewProjectionMatrix * vec4(worldPosition, 1.0f);
    for (int i = 0; i < poisson_size; i++) {
      vec2 rotatedOffset = rot * poisson[i];
      float lightOccluderDist = textureProj(map_shadow, projectedFrag + (vec4(rotatedOffset, 0, 0) / 1200.0f) + vec4(0, 0, offset, 0));
      shaded -= (1.0f - lightOccluderDist) * (1.0f / float(poisson_size));
    }

    // nice debug
    //shaded = pow(texture2D(map_depth, texCoord * 1.1f + vec2(0.05, 0.05)).r, 3.0f) * 3.0f - 0.6f;

    // how 'seriously' do we take shadows? various options below
    //shaded *= 0.25;
    //shaded += 0.75;
    //shaded *= 0.4;
    //shaded += 0.6;
    //shaded *= 0.5;
    //shaded += 0.5;
    shaded *= 0.85; // slightly deeper shadows for more contrast (PES style)
    shaded += 0.15;
  }

  vec3 fragColor = (base.rgb * lighted + specular) * shaded;

  // debug stuff
  //stdout0 = vec4(worldPosition.x * 0.01f + 0.05f, worldPosition.y * 0.01f + 0.05f, worldPosition.z * 0.01f, 0.0f);
  //stdout0 = vec4(worldPosition.xyz, 0.0f);
  //stdout0 = vec4(worldPosition.x * 0.005f, worldPosition.y * 0.005f, worldPosition.z * 0.05f - 5.0f, 0.0f);
  //gl_FragData[0] = vec4(vec3(fragDepth * 0.0025f), 0); //vec4(clamp(fragColor * brightness, 0.0, 1.0), 0.0) + vec4(lightColor.r, lightColor.g, lightColor.b, 0.0) * 0.1f;
  // position debug vec4(0.5 - base.r * 0.5, 0.5 - base.g * 0.5, 0.5 - base.b * 0.5, 0.0f);
  //gl_FragData[0] = vec4(vec3(inverseProjectionViewMatrix[3]) * 0.02f + 0.2f, 0.0f);
  stdout0 = vec4(fragColor, 0.0f);
  stdout1 = vec4(0);
}
