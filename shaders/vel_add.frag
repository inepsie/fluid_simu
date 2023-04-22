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
/* on reçoit les coordonnées autour desquelles on veut ajouter du mouvement */
uniform vec2 addsource;
/* on reçoit le vecteur de mouvement */
uniform vec2 motion; 
/* Le fragment shader est directement en relation avec les sorties du vertex shader */
in vec2 vsoTexCoord;
/* sortie du frament shader : une couleur */
out vec4 fragColor;
/*
void main(void) {
		vec2 source = step * addsource;
		float weight = 0.4 - sqrt(pow(vsoTexCoord.x - source.x, 2) + pow(vsoTexCoord.y - source.y, 2));
		if(weight<0.0) weight = 0.0;
		vec4 current = texture(tex, vsoTexCoord);
		fragColor = current;    
		fragColor = vec4(	4 * motion.x * step.x * weight + current.x,
						2 * motion.y * step.y * weight + current.y,
						0.0,
						0.0);
}
*/
void main(void) {
		vec2 source = step * addsource;
		float margeX = 30.0f * step.x;
		float margeY = 30.0f * step.y;
		vec4 current = texture(tex,vsoTexCoord);
		fragColor = current;
		if((vsoTexCoord.y < 1.0 - source.y + margeY) &&
						(vsoTexCoord.y > 1.0 - source.y - margeY) &&
						(vsoTexCoord.x < source.x + margeX) &&
						(vsoTexCoord.x > source.x - margeX)){
				fragColor = vec4(current.x - motion.x * step.x * 400.0, current.y + motion.y * step.y * 400.0, 0.0, 0.0);
		}
}
