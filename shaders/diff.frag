/*!\file diff.fs
 *
 fragment shader utilisé pour créer la diffusion de la matière dans la grille. Ce dernier utilise une méthode itérative de résolution d'équation (Gauss-Seidel), il doit donc être appelé un certain nombre de fois pour approximer les solutions.
/* Version GLSL 3.30 */
#version 330
/* texture 2 dimensions */
uniform sampler2D tex;
/* les pas pour passer au texel suivant ; soit en x soit en y */
uniform vec2 step;
/* valeur de perte */
uniform float loss;
/* variable pour savoir si on traite la densité ou la vitesse */
uniform int b;
/* texture boundaries */
uniform sampler2D tex_bnd;
/* Le fragment shader est directement en relation avec les sorties du vertex shader */
in vec2 vsoTexCoord;
/* sortie du frament shader : une couleur */
out vec4 fragColor;

void main(void) {
		// TODO w et h variables envoyées par le CPU
		float w = 800.0;
		float h = 800.0;
		float a = w * h * 0.1 * 0.01;
		vec2 offset = vec2(0.0, 0.0);
		vec2 inv = vec2(1.0, 1.0);
		if(bool(b)){
				a *= 0.01;
		}
		if(vsoTexCoord.x < step.x){
				offset = vec2(step.x, 0.0);
				inv = bool(b) ? vec2(-1.0, 1.0) : vec2(1.0, 1.0);
		}
		else if(vsoTexCoord.y < step.y){
				offset = vec2(0.0, step.y);
				inv = bool(b) ? vec2(1.0, -1.0): vec2(1.0, 1.0);
		}
		else if(vsoTexCoord.x > (1.0 - step.x)){
				offset = vec2(-step.x, 0.0);
				inv = bool(b) ? vec2(-1.0, 1.0) : vec2(1.0, 1.0);
		}
		else if(vsoTexCoord.y > (1.0 - step.y)){
				offset = vec2(0.0, -step.y);
				inv = bool(b) ? vec2(1.0, -1.0) : vec2(1.0, 1.0);
		}

		vec4 res =	( texture(tex, offset + vsoTexCoord) + a * (texture(tex, offset + vsoTexCoord + vec2(step.x, 0.0)) +
								texture(tex, offset + vsoTexCoord - vec2(step.x, 0.0)) +
								texture(tex, offset + vsoTexCoord + vec2(0.0, step.y)) +
								texture(tex, offset + vsoTexCoord - vec2(0.0, step.y))) ) / (1.0 + 4.0 * a); 
		res *= loss;
		fragColor =  vec4(inv.x * res.x, inv.y * res.y, res.z, 1.0);
		//Utiliser une texture pour aller chercher au bon endroit et eviter les if ?
}
