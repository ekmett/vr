#ifndef INCLUDED_SHADERS_SEASCAPE_GLSL
#define INCLUDED_SHADERS_SEASCAPE_GLSL

#include "brdf.glsl"
#include "spherical_harmonics.glsl"

vec2 iResolution = vec2(1024.,768.);

// based on https://www.shadertoy.com/view/Ms2SD1

// "Seascape" by Alexander Alekseev aka TDM - 2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// lighting model replaced

const int NUM_STEPS = 6;
const float PI         = 3.14159;
const float EPSILON    = 1e-3;
float EPSILON_NRM    = 0.1 / iResolution.x;

// sea
const int ITER_GEOMETRY = 3;
const int ITER_FRAGMENT = 6;

const float SEA_HEIGHT = 0.3;
const float SEA_CHOPPY = 2.0;
const float SEA_SPEED = 0.8;
const float SEA_FREQ = 0.16;

const vec3 SEA_BASE = vec3(0.1,0.19,0.22);

const vec3 SEA_WATER_COLOR = vec3(0.8,0.9,0.6);

float SEA_TIME = global_time * SEA_SPEED;

mat2 octave_m = mat2(1.6,1.2,-1.2,1.6);

float hash( vec2 p ) {
    float h = dot(p,vec2(127.1,311.7));
    return fract(sin(h)*43758.5453123);
}
float noise( in vec2 p ) {
    vec2 i = floor( p );
    vec2 f = fract( p );
    vec2 u = f*f*(3.0-2.0*f);
    return -1.0+2.0*mix( mix( hash( i + vec2(0.0,0.0) ),
                      hash( i + vec2(1.0,0.0) ), u.x),
                mix( hash( i + vec2(0.0,1.0) ),
                      hash( i + vec2(1.0,1.0) ), u.x), u.y);
}

// lighting
float diffuse(vec3 n,vec3 l,float p) {
  return pow(dot(n, l) * 0.4 + 0.6, p);
}
float specular(vec3 n,vec3 l,vec3 e,float s) {
    float nrm = (s + 8.0) / (3.1415 * 8.0);
    return pow(max(dot(reflect(e,n),l),0.0),s) * nrm;
}

// sea
float sea_octave(vec2 uv, float choppy) {
    uv += noise(uv);
    vec2 wv = 1.0-abs(sin(uv));
    vec2 swv = abs(cos(uv));
    wv = mix(wv,swv,wv);
    return pow(1.0-pow(wv.x * wv.y,0.65),choppy);
}

float map(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz; uv.x *= 0.75;

    float d, h = 0.0;
    for(int i = 0; i < ITER_GEOMETRY; i++) {
        d = sea_octave((uv+SEA_TIME)*freq,choppy);
        d += sea_octave((uv-SEA_TIME)*freq,choppy);
        h += d * amp;
        uv *= octave_m; freq *= 1.9; amp *= 0.22;
        choppy = mix(choppy,1.0,0.2);
    }
    return p.y - h;
}

float map_detailed(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz; uv.x *= 0.75;

    float d, h = 0.0;
    for(int i = 0; i < ITER_FRAGMENT; i++) {
        d = sea_octave((uv+SEA_TIME)*freq,choppy);
        d += sea_octave((uv-SEA_TIME)*freq,choppy);
        h += d * amp;
        uv *= octave_m; freq *= 1.9; amp *= 0.22;
        choppy = mix(choppy,1.0,0.2);
    }
    return p.y - h;
}

vec3 getSeaColor(vec3 p, vec3 N, vec3 L, vec3 I, vec3 dist) {
  vec3 V = -I;
  float NdV = saturate(dot(N, V));
  vec3 R = reflect(I, N);
  /*
  if (use_sun_area_light_approximation != 0) {
    float c = cos_sun_angular_radius;
    float s = sin_sun_angular_radius;
    float LdR = dot(L, R);
    L = LdR < c ? normalize(c * L + s * normalize(R - LdR * L)) : R;
  }
  */
  float atten = max(1.0 - dot(dist, dist) * 0.001, 0.0);
  vec3 refracted = mix(SEA_BASE,SEA_WATER_COLOR, 0.4*atten) * eval_sh9_irradiance(N, sky_sh9) / 3.14159; // diffuse/"ambient"
  vec3 reflected = R.y > 0 ? mix(SEA_BASE, SEA_WATER_COLOR, 0.5*atten) * getSkyColor(R) / 3.14159 : refracted;
  vec3 color = mix(reflected, refracted, F_schlick(f0_water, NdV)); // sun independent factors

  // sunlight
  vec3 H = normalize(V + L);
  float NdH = saturate(dot(N, H));
  float NdL = saturate(dot(N, L));
  float LdH = saturate(dot(L, H));
  if (NdL >= 0) {    
    vec3 F = F_schlick(f0_water, LdH);
    float smoothness = calc_smoothness(0.01);
    float D = D_ggx(smoothness, NdH);
    float G = G_ggx(smoothness, NdL, NdV);
    color += NdL * F * D * G * sun_irradiance / (turbidity*turbidity);
  }
  return color;
}

// tracing
vec3 getNormal(vec3 p, float eps) {
    vec3 n;
    n.y = map_detailed(p);
    n.x = map_detailed(vec3(p.x+eps,p.y,p.z)) - n.y;
    n.z = map_detailed(vec3(p.x,p.y,p.z+eps)) - n.y;
    n.y = eps;
    return normalize(n);
}

vec3 heightMapTracing(vec3 ori, vec3 dir) {
    float tm = 0.0;
    float tx = 1000.0;
    vec3 p = ori + dir * tx;
    float hx = map(p);
    if (hx > 0.0) return p;
    float hm = map(ori + dir * tm);
    float tmid = 0.0;
    for(int i = 0; i < NUM_STEPS; i++) {
        tmid = mix(tm,tx, hm/(hm-hx));
        p = ori + dir * tmid;
        float hmid = map(p);
        if(hmid < 0.0) {
            tx = tmid;
            hx = hmid;
        } else {
            tm = tmid;
            hm = hmid;
        }
    }
    return p;
}

#endif