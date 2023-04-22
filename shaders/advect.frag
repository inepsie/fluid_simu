/*!\file advect.fs
 *
 fragment shader utilisé pour créer la diffusion de la matière dans la grille. Ce dernier utilise une méthode itérative de résolution d'équation (Gauss-Seidel), il doit donc être appelé un certain nombre de fois pour approximer les solutions.
/* Version GLSL 3.30 */
#version 330
/* texture 2 dimensions pour la densité */
uniform sampler2D tex;
/* les pas pour passer au texel suivant ; soit en x soit en y */
uniform vec2 step;
/* texture 2 dimensions pour les vecteurs d'advection en x */
uniform sampler2D tex_advect;
/* variable pour savoir si on traite la densité ou la vitesse */
uniform int b;
/* texture boundaries */
uniform sampler2D tex_bnd;
/* Le fragment shader est directement en relation avec les sorties du vertex shader */
in vec2 vsoTexCoord;
/* sortie du frament shader : une couleur */
out vec4 fragColor;

void main(void) {
		/* On veut obtenir nouvelle couleur du pixel après que la matière ce soir déplacée selon le champs de vecteurs.
			 Le vecteur(x,y) permettant de calculer cette couleur pointe entre 4 pixels. La nouvelle couleur sera donc une
			 interpolation bilinéaire des 4 pixels (p0, p1, p2, p3) qui entourent la où pointe le vecteur.
		 */  

		vec4 bnd = texture(tex_bnd, vsoTexCoord);
		vec2 offset = bnd.xy * step;
		offset = vec2(0.0);

		float i0, i1, j0, j1, s1, s0, t1, t0;
		float dt0 = 800.0 * 0.01;		
		float x = (offset.x + vsoTexCoord.x) * 800.0 - dt0 * texture(tex_advect, offset + vsoTexCoord).x;
		float y = (offset.y + vsoTexCoord.y) * 800.0 - dt0 * texture(tex_advect, offset + vsoTexCoord).y;
		if(x<0.5) x = 0.5; else if(x > 800.0 + 0.5) x = 800.0 + 0.5; i0 = floor(x); i1 = i0 + 1;
		if(y<0.5) y = 0.5; else if(y > 800.0 + 0.5) y = 800.0 + 0.5; j0 = floor(y); j1 = j0 + 1;
		s1 = x - i0; s0 = 1 - s1; t1 = y - j0; t0 = 1 - t1;
		vec4 p00 = texture(tex, vec2(i0, j0) * step);
		vec4 p01 = texture(tex, vec2(i0, j1) * step);
		vec4 p10 = texture(tex, vec2(i1, j0) * step);
		vec4 p11 = texture(tex, vec2(i1, j1) * step);
		fragColor = s0 * (t0*p00 + t1*p01) + s1 * (t0*p10 + t1*p11);
		//fragColor = vec4(bnd.z * fragColor.x, bnd.w * fragColor.y, fragColor.z, 1.0);
		fragColor = vec4(fragColor.x, fragColor.y, fragColor.z, 1.0);
		/*
			 float vect_x = texture(tex_advect, offset + vsoTexCoord).x;
			 float vect_y = texture(tex_advect, offset + vsoTexCoord).y;
			 float x_floor = floor(vect_x);
			 float y_floor = floor(vect_y); 
			 float x_fract = vect_x - x_floor;
			 float y_fract = vect_y - y_floor;

			 vec4 p0 = texture(tex, offset + vsoTexCoord + x_floor * vec2(step.x, 0) + y_floor * vec2(0, step.y)); 
			 vec4 p1 = texture(tex, offset + vsoTexCoord + (x_floor+1) * vec2(step.x, 0) + y_floor * vec2(0, step.y)); 
			 vec4 p2 = texture(tex, offset + vsoTexCoord + (x_floor) * vec2(step.x, 0) + (y_floor+1) * vec2(0, step.y)); 
			 vec4 p3 = texture(tex, offset + vsoTexCoord + (x_floor+1) * vec2(step.x, 0) + (y_floor+1) * vec2(0, step.y)); 

			 vec4 lerp_p0p1 = p0 + x_fract * (p1 - p0);
			 vec4 lerp_p2p3 = p2 + x_fract * (p3 - p2);

			 vec4 res = lerp_p0p1 + y_fract * (lerp_p2p3 - lerp_p0p1);
			 fragColor = vec4(inv.x * res.x, inv.y * res.y, res.z, 1.0f);
		 */
}
