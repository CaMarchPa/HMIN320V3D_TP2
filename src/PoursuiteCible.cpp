/*
 *  PoursuiteCible.cpp
 *  
 *
 *  Created by Olivier Strauss on 17/10/16.
 *  Copyright 2016 LIRMM. All rights reserved.
 *
 */


#define margeDeRecherche 15
#define cimg_use_tif

#include "CImg.h"
#include "string.h"
#include "stdio.h"
#include "TraitementImage.h"
#include <utility>
#include <vector>

using namespace cimg_library;


std::pair<double,double> calculateDeltas(const CImg<unsigned char> &Image_lue, const int x0, const int x1, const int y0, const int y1, double* Valeur_motif);


int main(int argc, char *argv[])
{
	int nombre_de_points, n, increment, m ;
	
	const unsigned char red[] = { 255,0,0 }, green[] = { 0,255,0 }, blue[] = { 0,0,255 };
	
	char NomFichier[1024], *pt, *ptc, *fin ;
	char NomDeBase[1024], Suffixe[32], ok ;
	FILE *fichier ;
	
	CImg<unsigned char> Image_lue ;
	CImgDisplay Image_disp, Detail_disp ;
	CImg <unsigned char>::iterator Valeur_image ; 
	CImg <unsigned char>::iterator Valeur_detail ; 
	CImg <double>::iterator pointeur ; // pointeur de recuperation des valeurs de niveau de gris de l'image
	CImg <unsigned char> Detail ; // recuperation du motif en poursuite

	int carre_x[2], carre_y[2], nx, ny, dim ;
	int Nx, Ny ;	
	double delta_x=0.0, delta_y=0.0, dx, dy ;
	int x, y ;

	double *Image, *Image_x, *Image_y, *Image_transformee, *pti, *ptm, *finm ; 
	double *Valeur_motif, *Derivee_temporelle; 
	int *Point_x, *Point_y ;
	
	if(argc<1) return 0 ;	
	
 	strcpy(NomFichier,argv[1]) ;
	printf("\nNom = %s\n\n",NomFichier);
	fin = strrchr(NomFichier,'.') ;
	strcpy(Suffixe,fin) ;
	printf("suffixe = %s\n\n",Suffixe);
	fin -- ;
	while( ((*fin)>='0') && ((*fin)<='9') && fin>NomFichier ) fin -- ;
	fin ++ ;
	printf("suffixe = %s\n\n",fin);
	
	ptc = NomDeBase ;
	pt = NomFichier ;
	while(pt<fin) (*ptc++) = (*pt++) ; (*ptc) = (char)0 ;
	printf("NomDeBase = %s\n\n",NomDeBase);
	
	ok = 1 ;
	increment = 1 ;
	
	while(ok)
	{
		// Lecture de l'image courante
		if(increment<10)
			sprintf(NomFichier, "%s00%d%s",NomDeBase, increment, Suffixe) ;
		else
			if(increment<100)
				sprintf(NomFichier, "%s0%d%s",NomDeBase, increment, Suffixe) ;
		else 
			sprintf(NomFichier, "%s%d%s",NomDeBase, increment, Suffixe) ;

		// tentative pour ouvrir le fichier (pour voir s'il existe)
		fichier = fopen(NomFichier,"r") ;
		
		ok = (fichier!=NULL) ;
		if(ok) fclose(fichier) ; 
		
		if(ok) // si le fichier existe lire l'image
		{ // et l'afficher dans une fenetre
			Image_lue.load(NomFichier) ;
			Image_disp.display(Image_lue) ; 
		}
		
		if(increment==1) // s'il s'agit de la premiere image
		{
			Nx = Image_lue.width() ;
			Ny = Image_lue.height() ;			
			
			Image = ALLOCATION(Nx*Ny,double); 
			Image_x = ALLOCATION(Nx*Ny,double); 
			Image_y = ALLOCATION(Nx*Ny,double); 		
			Image_transformee = ALLOCATION(Nx*Nx,double) ;		
		}
	
		Valeur_image = Image_lue.begin() ;
		pti = Image ;
		for(y=0 ; y<Ny ; y++)
		{
			for(x=0 ; x<Nx ; x++)
			{
				(*pti++) = (double)(*Valeur_image++) ;
			}
		}
							
		nombre_de_points = 4 ;
		
		if(increment==1)
		{ 
			// selection du motif
			
			nombre_de_points = Max(nombre_de_points,3) ;
			// au moins trois point pour définir un carre
			
			while (!Image_disp.is_closed() && n<nombre_de_points)
			{
				Image_disp.set_title("%s","selectionner une zone");
				Image_disp.wait();
				if (Image_disp.button() && Image_disp.mouse_y()>=0) 
				{
					y = Image_disp.mouse_y();
					x = Image_disp.mouse_x();
					Image_lue.draw_circle(x,y,1,red).display(Image_disp);
					if(n>1)
					{
					 int k ;
						carre_x[0] = carre_x[0]<x ? carre_x[0] : x ;
						carre_x[1] = carre_x[1]>x ? carre_x[1] : x ;
						carre_y[0] = carre_y[0]<y ? carre_y[0] : y ;
						carre_y[1] = carre_y[1]>y ? carre_y[1] : y ;
					}
					else 
					{
					 carre_x[0] = x ;
					 carre_x[1] = x ;
					 carre_y[0] = y ;
					 carre_y[1] = y ;
					}
					n++ ;
				}
		 }
			
			Image_disp.set_title(" .oOo. ");

			nx = carre_x[1] - carre_x[0] ;
			ny = carre_y[1] - carre_y[0] ;
			
			dim = nx*ny ;

			if(dim == 0) return 0 ;
			
			Valeur_motif = ALLOCATION(dim, double) ;
			Derivee_temporelle = ALLOCATION(dim, double) ;
			Point_x = ALLOCATION(dim, int) ;
			Point_y = ALLOCATION(dim, int) ;
			
			// Derivation de la premiere image
			Derive_image( Image, Image_x, Image_y, Nx, Ny, 0.4 ) ;
						
			delta_x = 0.0 ;
			delta_y = 0.0 ;
			
			Detail= Image_lue ; // delimitation du detail pour affichage
			Detail.crop(carre_x[0], carre_y[0], carre_x[1], carre_y[1]) ;
			
			// rechargement de l'image originale
			Image_lue.load(NomFichier) ;
			Image_disp.display(Image_lue) ;
			
			// conservation des points du motif
			// leur valeur de niveau de gris dans Valeur_motif
			// et leur coordonnees dans Point_x et Point_y
			for( y=carre_y[0], m=0 ; y<carre_y[1] ; y++)
			{
				for( x=carre_x[0] ; x<carre_x[1] ; x++, m++)
				{		
 				n = y*Nx+x ;
					Valeur_motif[m] = Image[n] ;
					Point_x[m] = x ;
					Point_y[m] = y ;
				}
			}
		} // fin de if increment == 1

		else 
		{

			Valeur_image = Image_lue.begin() ;
		 	Detail= Image_lue ;
			Detail.crop(carre_x[0],carre_y[0],carre_x[1],carre_y[1]) ;
			Valeur_detail = Detail.begin() ;
			pti = Valeur_motif ;
		
			for(n=0 ; n<dim ; n++, Valeur_detail++, pti++)
 		{
		  m=Point_y[n]*Nx+Point_x[n];
				Derivee_temporelle[n] = (*pti) - Image_transformee[m] ;					
			}
			
			// C'est la que vous devez mettre a jour
			// la position du motif dans l'image courante

			std::pair<double, double> deltas = calculateDeltas(Image_lue, carre_x[0], carre_x[1], carre_y[0], carre_y[1], Valeur_motif);
			
 			delta_x = deltas.first;
 			delta_y = deltas.second;

 			carre_x[0] += delta_x;
 			carre_x[1] += delta_x;
 			carre_y[0] += delta_y;
			carre_y[1] += delta_y;
 		// delta_x += 0.0 ;
 		// delta_y += 0.0 ;
		}
	
			
		Detail.resize(4*nx, 4*ny) ;
		Detail_disp.display(Detail) ;
		
		Image_lue.draw_rectangle(carre_x[0],carre_y[0],carre_x[1],carre_y[1],blue,0.3).display(Image_disp);
		
	 // pour que l'image ne s'incremente que si on passe la souris sur la fenetre
		Image_disp.wait();
	 while(Image_disp.button()) ; 
	
		increment ++ ;
		//increment = Min(increment,3) ;

	}
	
 DESALLOCATION(Image) ;
 DESALLOCATION(Image_x) ;
 DESALLOCATION(Image_y) ;
 DESALLOCATION(Image_transformee) ;
 DESALLOCATION(Valeur_motif) ;
 DESALLOCATION(Derivee_temporelle) ;
 DESALLOCATION(Point_x) ;
 DESALLOCATION(Point_y) ;
		
	return 0 ;

}

std::pair<double,double> calculateDeltas(const CImg<unsigned char> &Image_lue, const int x0, const int x1, const int y0, const int y1, double* Valeur_motif) {
	// On cherche dans l'image un motif potentiel qui maximisera la correlation avec le motif précédent (valeur_motif)

	std::pair<double,double> deltas;
	double correlation = 0.0;

	for (int dy = -margeDeRecherche; dy < margeDeRecherche + 1; dy++) { 
		for (int dx = -margeDeRecherche; dx < margeDeRecherche + 1; dx++) {
			// On cherche le motif potentiel à proximité du motif précédent, dans un rayon définit par margeDeRecherche.

			CImg <unsigned char> motif_Potentiel_CImg = Image_lue;
			motif_Potentiel_CImg.crop(x0 + dx, y0 + dy, x1 + dx, y1 + dy); // Le motif potentiel

			// Conversion du motif potentiel en tableau
			int nx = x1 - x0 + 1;
			int ny = y1 - y0 + 1;
			std::vector<double> motif_Potentiel(nx*ny);
			for(int i = 0; i < nx*ny; i++) {
				motif_Potentiel[i] = motif_Potentiel_CImg[i];
			}

			// calcul de la correlation=
			double c = CorrelationPearson(Valeur_motif, motif_Potentiel.data(), nx, ny);

			// On garde le motif dont la correlation est maximale.
			if (c > correlation) {
				correlation = c;
				deltas.first = dx;
				deltas.second = dy;
			}
		}
	}

	return deltas;
}