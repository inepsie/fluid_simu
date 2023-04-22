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
/* texture div */
uniform sampler2D tex_div;
/* texture boundaries */
uniform sampler2D tex_bnd;
/* Le fragment shader est directement en relation avec les sorties du vertex shader */
in vec2 vsoTexCoord;
/* sortie du frament shader : une couleur */
out vec4 fragColor;

void main(void) {
  //Utiliser une texture pour aller chercher au bon endroit et eviter les if ?
  vec4 bnd = texture(tex_bnd, vsoTexCoord);
  vec2 offset = bnd.xy * step;
  fragColor = vec4((texture(tex_div, offset + vsoTexCoord).x + 1.01 * (
                    texture(tex, offset + vsoTexCoord - vec2(step.x, 0)).x +
                    texture(tex, offset + vsoTexCoord + vec2(step.x, 0)).x+
                    texture(tex, offset + vsoTexCoord - vec2(0, step.y)).x +
                    texture(tex, offset + vsoTexCoord + vec2(0, step.y)).x)
								) / 4.0,
						0.0, 0.0, 0.0);
}

