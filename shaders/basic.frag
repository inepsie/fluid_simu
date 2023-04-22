/*!\file basic.fs
 *
 * \brief fragment shader basique qui applique une texture en se
 * décalant d'un texel. Comme cela est fait de manière cyclique, nous
 * obtenons une translation de la texture.
 * \author Farès BELHADJ, amsi@up8.edu
 * \date July 15 2020
 */
/* Version GLSL 3.30 */
#version 330
/* texture 2 dimensions */
uniform sampler2D tex;
/* les pas pour passer au texel suivant ; soit en x soit en y */
uniform vec2 step;
/* on reçoit les coordonnées du pixel dont on veut changer la couleur */
uniform vec2 addsource;
/* Le fragment shader est directement en relation avec les sorties du vertex shader */
in vec2 vsoTexCoord;
/* sortie du frament shader : une couleur */
out vec4 fragColor;

void main(void) {
		float margeX = 40.0f * step.x;
		float margeY = 40.0f * step.y;
		fragColor = texture(tex, vsoTexCoord);
		if((vsoTexCoord.y < 1.0 - addsource.y * step.y + margeY) &&
						(vsoTexCoord.y > 1.0 - addsource.y * step.y - margeY) &&
						(vsoTexCoord.x < addsource.x * step.x + margeX) &&
						(vsoTexCoord.x > addsource.x * step.x - margeX)){
				//fragColor = texture(tex, vsoTexCoord) + (vec4(1.0) - texture(tex, vsoTexCoord)) * 0.2;
				fragColor = vec4(0.9, 0.9, 0.9, 1.0);
		}

}
