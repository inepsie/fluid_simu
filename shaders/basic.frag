#version 430

uniform sampler2D tex;
uniform float nl;

in vec2 vsoTexCoord;

out vec4 fragColor;



void main(void) {
 fragColor = texture(tex, vsoTexCoord);
 //float l = (fragColor.x + fragColor.y + fragColor.z) / 3.0;
 //l = pow(l, nl);
 fragColor.x = pow(fragColor.x, nl);
 fragColor.y = pow(fragColor.y, nl);
 fragColor.z = pow(fragColor.z, nl);
 //fragColor = texture(tex, vsoTexCoord);
}
