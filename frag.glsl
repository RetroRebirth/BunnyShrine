uniform mat4 uP;
uniform mat4 uV;
uniform mat4 uM;
uniform mat4 uR;
uniform int uID;
uniform vec3 uView;
uniform vec3 uL1;
uniform vec3 uL2;
uniform vec3 uL3;
uniform vec3 uAClr;
uniform vec3 uDClr;
uniform vec3 uSClr;
uniform float uS;
varying vec4 vPos;
varying vec4 vNor;

float rand(vec2 co) {
   return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

// Averages the value with adjacent seeds
float noise(vec2 co) {
   float total = 0.0;

   for (int i = co.x-1; i < co.x+1; i++) {
      for (int j = co.y-1; j < co.y+1; j++) {
         if (i == co.x && j == co.y) {
            total += rand(vec2(i, j))/2;
         } else {
            total += rand(vec2(i, j))/16;
         }
      }
   }

   return total;
}

void main() {
   vec3 col;

   vec3 pos = vec3(uM * vPos);
   vec3 nor = vec3(uR * vNor);
   vec3 view = uView - pos;

   // Light vector and dot product (no light if facing away)
   vec3 lightV1 = normalize(uL1 - pos); // point source light (light bulb)
   vec3 lightV2 = normalize(uL2 - pos); // point source light (light bulb)
   vec3 lightV3 = normalize(uL3 - pos); // point source light (light bulb)
   float light1 = max(0.0, dot(normalize(nor), normalize(lightV1)));
   float light2 = max(0.0, dot(normalize(nor), normalize(lightV2)));
   float light3 = max(0.0, dot(normalize(nor), normalize(lightV3)));

   vec3 H1 = normalize(view + lightV1);
   vec3 H2 = normalize(view + lightV2);
   vec3 H3 = normalize(view + lightV3);
   float h1 = pow(max(0.0, dot(normalize(nor), H1)), uS);
   float h2 = pow(max(0.0, dot(normalize(nor), H2)), uS);
   float h3 = pow(max(0.0, dot(normalize(nor), H3)), uS);

   // Diffuse light
   vec3 dClr1 = light1 * uDClr;
   vec3 dClr2 = light2 * uDClr;
   vec3 dClr3 = light3 * uDClr;

   // Ambient light
   vec3 aClr = uAClr;

   // Specular light
   vec3 sClr1 = light1 * h1 * uSClr;
   vec3 sClr2 = light2 * h2 * uSClr;
   vec3 sClr3 = light3 * h3 * uSClr;

   col = dClr1 + dClr2 + dClr3 + aClr + sClr1 + sClr2 + sClr3;

   switch(uID) {
   case 1:
      float noise = noise(vec2(pos.x, pos.z));

      col.r = noise*(0.31) + (1-noise)*(0.34);
      col.g = noise*(0.78) + (1-noise)*(0.23);
      col.b = noise*(0.47) + (1-noise)*(0.05);
      break;
   case 2:
      float grad = pos.y/10;

      col.r = grad*(0.52) + (1-grad)*(0.8);
      col.g = grad*(0.80) + (1-grad)*(0.8);
      col.b = grad*(0.98) + (1-grad)*(0.8);
      break;
   }

   gl_FragColor = vec4(col, 1.0);
}
