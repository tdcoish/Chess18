/***********************************************************************************
This is a chess program that creates a chess board with controlled random positioning
of the back row of pieces. The king and rooks will always be in the same position, however,
all other pieces will be randomized. The only exception, is that a light and dark square
bishop is required. All told, there are 18 different starting positions, excluding repeats. 
So this version of chess might be called chess 18. If we don't want to mirror the positions,
then we can call this chess 324.
***********************************************************************************/


#include <iostream>
#include <cstdlib>
#include "time.h"
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb-master\stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb-master\stb_image_write.h"

#include "SDL2/SDL.h"
//will need SDL_IMG library for this
#include <SDL2/SDL_image.h>

SDL_Window* pWin=NULL;
SDL_Renderer* pRend=NULL;

struct Pixel{
	unsigned char r, g, b, a;
};

//goddamn Pixel* caused me lots of problems.
struct Image{
	int wd, ht, bpp;
	Pixel* pImg;
};

struct Graphic{
	Image img;
	std::string type;
};


Graphic imgs[13];
Image FINAL_BOARD;
const int BOARD_SIZE=500;		//500x500 pixels.

float scale;		//used for graphics.

bool Init(SDL_Window** ppWin, SDL_Renderer** ppFrame)
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		printf("Initialization failure. Error: %s\n", SDL_GetError());
		return false;
	}
	if(SDL_Init(SDL_INIT_TIMER)<0){
		printf("Error initializing timer. Error: %s\n", SDL_GetError());
		return false;
	}

	//Set texture filtering to linear
	if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")){
		printf("Linear texture filtering not enabled\n");
		return false;
	}
	if(!SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0")){
		printf("Failed to ovveride vsync\n");
		return false;
	}

	*ppWin=SDL_CreateWindow("Chess Program", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 800, SDL_WINDOW_RESIZABLE);
	if(*ppWin==NULL){
		printf("Error: %s creating window.\n", SDL_GetError());
		return false;
	}

	scale=800/100;

	*ppFrame=SDL_CreateRenderer(*ppWin, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	if(*ppFrame==NULL){
		printf("Renderer creation failure. Error: %s\n", SDL_GetError());
		return false;
	}

	//Set background colour. Basically GLClearColor
	SDL_SetRenderDrawColor(*ppFrame, 0xFF, 0xFF, 0xFF, 0xFF);

	//eventually need to load PNG's into SDL2
	int imgFlags = IMG_INIT_PNG;
	if(!(IMG_Init(imgFlags)&imgFlags)){
		printf("SDL_image not initialized. Error: %s\n", IMG_GetError());
		return false;
	}
	

	return true;

	SDL_SetRelativeMouseMode(SDL_FALSE);
}

void PickPieces(bool mirrored);

enum Backrow{KING, QUEEN, ROOK, BISHOP, KNIGHT, AMOUNT};

void PrintRows();

int whtPieces[8];
int blkPieces[8];

///Load in a PNG to our Image struct
bool LoadImage(Image* pImg, std::string filePath);

//blit image 2 onto a percent of image 1.
void ScaleApplyImage(Image* pI1, Image* pI2, int startX, int startY, float size);

//blit one image onto another.
void ApplyImage(Image* pI1, Image* pI2, int startX, int startY, float size);

//Wrap around stbi_write_png to write our png again.
void WriteImage(Image img, std::string name);

//If we want to manually create the raw pixel data. Must be square
void CreateImage(Image* pImg, int size);

//create a new size using nearest neighbour.
Image ResizeImage(Image old, float size);

//delete allocated pixels inside Image struct.
void DeleteImage(Image img);

void LoadImages(std::string path);

///load in a PNG and convert it to a texture.
bool LoadTexture(std::string path, SDL_Texture** ppTextImg, SDL_Renderer* pRend);

void LoadTextures(std::string path);

///Only works with square textures.
///pSrcClp will usually be NULL.
void ScaleApplyTexture(SDL_Texture* pSrc, SDL_Renderer* pFrame, SDL_Rect* pSrcClp, int x, int y, int size);

int FindPawn(bool white);

void CreateBoardPNG();

//apparently fMod gives me this functionatlity.
int RoundFloatUpOrDown(float num){
	int temp=(int)num;
	float temp2=(float)temp;
	if(num-temp2>0.5){
		return (int)(num+1);
	}else{
		return (int) num;
	}
}

std::string bsPth=SDL_GetBasePath();
std::string imgPath(bsPth+"Chess Pieces");

int main(int args, char** argc)
{
	Init(&pWin, &pRend);
	CreateImage(&FINAL_BOARD, BOARD_SIZE);
	bool mirrored=false;
	PickPieces(mirrored);

	std::size_t pos=bsPth.find("Debug");
	std::string savePath=bsPth.substr(0, pos);
	savePath+="New Chess//";
	std::cout << savePath;

	LoadImages(imgPath);

	CreateBoardPNG();

	SDL_Texture* pBoard=NULL;
	if(!LoadTexture(savePath+"//full board.png", &pBoard, pRend))
		return -1;
	bool reloadTexture=false;
	
	
	SDL_Event e;
	bool quit=false;
	while(!quit){
		//if user left clicks, generate new board.
		//if user right clicks, close application.
		while(SDL_PollEvent(&e)!=0){
			if(e.type==SDL_MOUSEBUTTONDOWN){
				if(e.button.button==SDL_BUTTON_LEFT){
					//generate new board
					PickPieces(mirrored);
					PrintRows();
					CreateBoardPNG();
					reloadTexture=true;
				}else if (e.button.button==SDL_BUTTON_RIGHT){
					quit=true;
				}
			}
			if(e.type==SDL_QUIT){
				quit=true;
			}
		}

		//Render Board
		SDL_SetRenderDrawColor(pRend, 255, 255, 255, 255);
		SDL_RenderClear(pRend);

		if(reloadTexture){
			SDL_DestroyTexture(pBoard);
			pBoard=NULL;
			if(!LoadTexture(savePath+"//full board.png", &pBoard, pRend))
				return -1;
			reloadTexture=true;
		}

		ScaleApplyTexture(pBoard, pRend, NULL, 0, 0, 100);

		SDL_RenderPresent(pRend);

		//delete pBoard;

		SDL_Delay(20);
	}	

	return 0;
}

bool LoadImage(Image* pImg, std::string filePath){
	unsigned char* pRaw=NULL;
	pRaw=stbi_load(filePath.c_str(), &pImg->wd, &pImg->ht, &pImg->bpp, 0);
	if(pRaw==NULL){
		std::cout << "Error loading PNG\n";
		return false;
	}

	
	//allocate memory for pixels
	int size=pImg->wd*pImg->ht;
	pImg->pImg=new Pixel[size]();
	//if image has no alpha channel, give it full opaqueness which == 255.
	for(int i=0; i<size*pImg->bpp; i+=pImg->bpp){
		pImg->pImg[i/pImg->bpp].r=pRaw[i];
		pImg->pImg[i/pImg->bpp].g=pRaw[i+1];
		pImg->pImg[i/pImg->bpp].b=pRaw[i+2];
		if(pImg->bpp==4){
			pImg->pImg[i/pImg->bpp].a=pRaw[i+3];

			//an ugly hack because paint turned all my alpha into white. This will ruin white images.
			if(pImg->pImg[i/pImg->bpp].r==255 && pImg->pImg[i/pImg->bpp].g==255 && pImg->pImg[i/pImg->bpp].r==255){
				pImg->pImg[i/pImg->bpp].a=0;		//make all white pixels transparent.
			}
		}else{
			pImg->pImg[i/pImg->bpp].a=255;		//fully opaque.
		}
	}
	pImg->bpp=4;		//since we've added the alpha channel.

	delete pRaw;
	return true;
}

//Loads the specific images that we need
void LoadImages(std::string path)
{
	imgs[0].type="WHITE KING";
	LoadImage(&imgs[0].img, path+"\\White King.png");

	imgs[1].type="WHITE QUEEN";
	LoadImage(&imgs[1].img, path+"\\White Queen.png");

	imgs[2].type="WHITE ROOK";
	LoadImage(&imgs[2].img, path+"\\White Rook.png");

	imgs[3].type="WHITE BISHOP";
	LoadImage(&imgs[3].img, path+"/White Bishop.png");

	imgs[4].type="WHITE KNIGHT";
	LoadImage(&imgs[4].img, path+"/White Knight.png");

	imgs[5].type="BLACK KING";
	LoadImage(&imgs[5].img, path+"/Black King.png");

	imgs[6].type="BLACK QUEEN";
	LoadImage(&imgs[6].img, path+"/Black Queen.png");

	imgs[7].type="BLACK BISHOP";
	LoadImage(&imgs[7].img, path+"/Black Bishop.png");

	imgs[8].type="BLACK KNIGHT";
	LoadImage(&imgs[8].img, path+"/Black Knight.png");

	imgs[9].type="BLACK ROOK";
	LoadImage(&imgs[9].img, path+"/Black Rook.png");

	imgs[10].type="BOARD";
	LoadImage(&imgs[10].img, path+"/chess board.png");

	imgs[11].type="WHITE PAWN";
	LoadImage(&imgs[11].img, path+"/White Pawn.png");

	imgs[12].type="BLACK PAWN";
	LoadImage(&imgs[12].img, path+"/Black Pawn.png");
}

//using simple nearest neighbour.
//assumes square images.
Image ResizeImage(Image old, float size)
{
	//if the new size is the same as the old size.
	if(old.wd==size)
		return old;

	int count=0;
	Image temp;
	CreateImage(&temp, (int)size);

	//can't go past with the X value.

	//nearest neighbour algorithm.
	float increase=(float)old.wd/size;
	float oldIndice;
	int spotX, spotY;
	float oldSpotX, oldSpotY;
	int testDif=0;
	int numDif=0;
	for(int i=0; i<size*size; i++){
		spotX=i%(int)size;		//so if picture == 20x20 and i=42. x=2.
		spotY=i/size;			//so if picture==20x20 and i=42. y=2
		oldSpotX=(float)spotX*increase;
		//can't go past the edge.
		oldSpotX=(int)oldSpotX;
		oldSpotY=(float)spotY*increase;
		//always start at the beginning of the line
		oldSpotY=(int)oldSpotY;
		oldIndice=RoundFloatUpOrDown(oldSpotY*(float)old.wd+(oldSpotX));


		temp.pImg[i]=old.pImg[(int)oldIndice];

	}
	return temp;
}

void ScaleApplyImage(Image* pI1, Image* pI2, int startX, int startY, float size){
	float percent=(float)pI1->wd/100;
	startX*=percent;
	startY*=percent;
	size*=percent;
	ApplyImage(pI1, pI2, startX, startY, size);
}

//software blit one image onto another. Second image is being blitted into the first
void ApplyImage(Image* pI1, Image* pI2, int startX, int startY, float size){
	size=(int)size;
	Image temp;
	//resize the image here.
	temp=ResizeImage(*pI2, size);

	int startLoc=pI1->wd*startY+startX;


	//keep blitting in bounds
	int blitStartY=0;
	int blitStartX=0;
	if(startX<0)
		blitStartX-=startX;
	if(startY<0)
		blitStartY-=startY;

	int blitLengthX=size;
	if((startX+size)>pI1->wd){
		blitLengthX-=(startX+size)-pI1->wd;	//don't go past right edge
		blitLengthX-=blitStartX;		//don't go past left edge
	}
	int blitLengthY=size;
	if(startY+size>pI1->ht){
		blitLengthY-=(startY+size)-pI1->ht;
		blitLengthY-=blitStartY;	
	}
	
	//blit all valid pixels.
	int im1pxl, im2pxl;
	for(int line=blitStartY; line<blitLengthY; line++){
		for(int i=blitStartX; i<blitLengthX; i++){
			im1pxl=pI1->wd*line+i+startLoc;
			im2pxl=temp.wd*line + i;

			//if fully opaque, just write over.
			int alphaVal=(int) temp.pImg[im2pxl].a;
			if(alphaVal==255){
				pI1->pImg[im1pxl]=temp.pImg[im2pxl];
			}
			//if partially opaque, blend appropriately
			else if(alphaVal!=0){
				float newPercent=temp.pImg[im2pxl].a/255.0;
				float oldPercent=1.0-newPercent;
				Pixel newPixel;
				//newPixel.a=pI1->pImg[im1px].
				newPixel.r=pI1->pImg[im1pxl].r*oldPercent + temp.pImg[im2pxl].r*newPercent;
				newPixel.g=pI1->pImg[im1pxl].g*oldPercent + temp.pImg[im2pxl].g*newPercent;
				newPixel.b=pI1->pImg[im1pxl].b*oldPercent + temp.pImg[im2pxl].b*newPercent;
				//what should the alpha be?????? 
				newPixel.a=255;		//end result should always be opaque, since I assume the background is opaque anyway.

				pI1->pImg[im1pxl]=newPixel;
			}
			//pixel is translucent
			else{
				//just do nothing, since we don't blend.
			}
		}
	}
}

//Wrap around stbi_write_png to write our png again.
void WriteImage(Image img, std::string name){
	int size=img.ht*img.wd*img.bpp;
	unsigned char* pRaw=new unsigned char[size]();
	for(int i=0; i<size; i+=img.bpp){
		pRaw[i]=img.pImg[i/img.bpp].r;
		pRaw[i+1]=img.pImg[i/img.bpp].g;
		pRaw[i+2]=img.pImg[i/img.bpp].b;
		pRaw[i+3]=img.pImg[i/img.bpp].a;
	}
	stbi_write_png(name.c_str(), img.wd, img.ht, img.bpp, pRaw, 0);
	delete pRaw;		//for some reason this is giving me an error.
}

//If we want to manually create the raw pixel data.
void CreateImage(Image* pImg, int size){
	pImg->ht=size;
	pImg->wd=size;
	pImg->bpp=4;			//all images have alpha channel.
	pImg->pImg=NULL;
	int memSize=size*size;
	pImg->pImg=new Pixel[memSize]();
}

//delete allocated pixels inside Image struct.
void DeleteImage(Image img){
	delete img.pImg;
}


//just finds the right graphic location
int ChoosePieceToRender(int spot, bool white)
{
	if(white){
		if(whtPieces[spot]==KING){
			for(int i=0; i<5; i++){
				if(imgs[i].type=="WHITE KING")
					return i;
			}
		}
		if(whtPieces[spot]==QUEEN){
			for(int i=0; i<5; i++){
				if(imgs[i].type=="WHITE QUEEN")
					return i;
			}
		}
		if(whtPieces[spot]==BISHOP){
			for(int i=0; i<5; i++){
				if(imgs[i].type=="WHITE BISHOP")
					return i;
			}
		}
		if(whtPieces[spot]==KNIGHT){
			for(int i=0; i<5; i++){
				if(imgs[i].type=="WHITE KNIGHT")
					return i;
			}
		}
		if(whtPieces[spot]==ROOK){
			for(int i=0; i<5; i++){
				if(imgs[i].type=="WHITE ROOK")
					return i;
			}
		}
	}
	//Now the black pieces
	if(!white){
		if(blkPieces[spot]==KING){
			for(int i=0; i<11; i++){
				if(imgs[i].type=="BLACK KING")
					return i;
			}
		}
		if(blkPieces[spot]==QUEEN){
			for(int i=0; i<11; i++){
				if(imgs[i].type=="BLACK QUEEN")
					return i;
			}
		}
		if(blkPieces[spot]==BISHOP){
			for(int i=0; i<11; i++){
				if(imgs[i].type=="BLACK BISHOP")
					return i;
			}
		}
		if(blkPieces[spot]==KNIGHT){
			for(int i=0; i<11; i++){
				if(imgs[i].type=="BLACK KNIGHT")
					return i;
			}
		}
		if(blkPieces[spot]==ROOK){
			for(int i=0; i<11; i++){
				if(imgs[i].type=="BLACK ROOK")
					return i;
			}
		}
	}
	return 0;
}


void ScaleApplyTexture(SDL_Texture* pSrc, SDL_Renderer* pFrame, SDL_Rect* pSrcClp, int x, int y, int size)
{
    SDL_Rect dstClp;
	dstClp.x = x*scale;
	dstClp.y = y*scale;
    dstClp.w = size*scale;
    dstClp.h = size*scale;

    SDL_RenderCopy(pFrame, pSrc, pSrcClp, &dstClp);
}

int FindPawn(bool white){
	if(white){
		for(int i=0; i<13; i++){
			if(imgs[i].type=="WHITE PAWN"){
				return i;
			}
		}
	}
	
	else{
		for(int i=0; i<13; i++){
			if(imgs[i].type=="BLACK PAWN")
				return i;
		}
	}
	return -1;
}

void CreateBoardPNG()
{
	//blit the white pieces
	float xOffset=0; float yOffset=87.5;
	int correctPiece;

	//blit the board as the background.
	ScaleApplyImage(&FINAL_BOARD, &imgs[10].img, 0, 0, 100);		//should really not use magic numbers for the board location.

	for(int i=0; i<8; i++){
		correctPiece=ChoosePieceToRender(i, true);
		ScaleApplyImage(&FINAL_BOARD, &imgs[correctPiece].img, xOffset, yOffset, 12.5);
		xOffset+=12.5;
	}
	//add pawns
	xOffset=0; yOffset=75;
	for(int i=0; i<8; i++){
		ScaleApplyImage(&FINAL_BOARD, &imgs[FindPawn(true)].img, xOffset, yOffset, 12.5);
		xOffset+=12.5;
	}
	//blit black pieces
	xOffset=0; yOffset=0;
	correctPiece=0;

	for(int i=0; i<8; i++){
		correctPiece=ChoosePieceToRender(i, false);
		ScaleApplyImage(&FINAL_BOARD, &imgs[correctPiece].img, xOffset, yOffset, 12.5);
		xOffset+=12.5;
	}
	//add pawns
	xOffset=0; yOffset=12.5;
	for(int i=0; i<8; i++){
		ScaleApplyImage(&FINAL_BOARD, &imgs[12].img, xOffset, yOffset, 12.5);
		xOffset+=12.5;
	}

	WriteImage(FINAL_BOARD, "full board.png");
}

///load in a PNG and convert it to a texture.
bool LoadTexture(std::string path, SDL_Texture** ppTextImg, SDL_Renderer* pRend)
{
    SDL_Surface* pImg = NULL;
    SDL_Texture* pTemp = NULL;
    pImg = IMG_Load(path.c_str());
    if (pImg == NULL){
        printf("PNG not loaded. IMG Error: %s\n", IMG_GetError());
        return false;
    }
    pTemp = SDL_CreateTextureFromSurface(pRend, pImg);
    if (pTemp == NULL){
        printf("Surface to texture conversion failed. Error: %s\n", SDL_GetError());
        return false;
    }
    //we again need to free the surface
    SDL_FreeSurface(pImg);

    *ppTextImg = pTemp;
    return true;
}

void PrintRows()
{
	int* pieces;
	pieces=whtPieces;
	std::cout << "-----------------------------------------\n";
	for(int i=0; i<8; i++){
		switch(pieces[i]){
			case KING: std::cout << "KING\n"; break;
			case QUEEN: std::cout << "QUEEN\n"; break;
			case ROOK: std::cout << "ROOK\n"; break;
			case BISHOP: std::cout << "BISHOP\n"; break;
			case KNIGHT: std::cout << "KNIGHT\n"; break;
			default: std::cout << "ERROR\n"; break;
		}
	}

	pieces=blkPieces;
	std::cout << "\nBlack Pieces:\n";
	for(int i=0; i<8; i++){
		switch(pieces[i]){
			case KING: std::cout << "KING\n"; break;
			case QUEEN: std::cout << "QUEEN\n"; break;
			case ROOK: std::cout << "ROOK\n"; break;
			case BISHOP: std::cout << "BISHOP\n"; break;
			case KNIGHT: std::cout << "KNIGHT\n"; break;
			default: std::cout << "ERROR\n"; break;
		}
	}
	std::cout << "-----------------------------------------\n";
}

//Update: now we generate one row at a time. This is because we may or may not mirror the board.
//Return a row.
void GenRow(int* pieces)
{
	//There are 8 pieces on the back row. Spots 1 and 8 are rooks. Spot 5 is a king (for white).
	//The above assumes left to right
	//These are the pieces that don't immediately have a home
	int movers[5];
	movers[0]=BISHOP;
	movers[1]=BISHOP;
	movers[2]=QUEEN;
	movers[3]=KNIGHT;
	movers[4]=KNIGHT;
	int unplaced=5;

	for(int i=0; i<8; i++){
		if(i==0 || i==7){
			pieces[i]=ROOK;
		}else if(i==4){
			pieces[i]=KING;
		}else{
			int piece=rand()%unplaced;
			pieces[i]=movers[piece];

			//delete the picked piece, so it is not picked again.
			movers[piece]=AMOUNT;

			//shuffle the remaining pieces down into a smaller effective array
			piece++;
			while(piece<unplaced){
				movers[piece-1]=movers[piece];
				piece++;
			}

			//one less valid piece to select from
			unplaced--;
		}
	}
	return;
}

bool CheckRow(int* pieces)
{
	//now check that the bishops are on odd, and even indexes. Otherwise, repeat the whole process.
	
	//find which indices are bishops
	int bishop1=0; 
	int bishop2=0;
	bool first=true;
	for(int i=0; i<8; i++){
		if(pieces[i]==BISHOP){
			if(first){
				bishop1=i;
				first=false;
			}else if (!first){
				bishop2=i;
			}
		}
	}

	//check that they're not both even, or both odd.
	int distance=bishop2-bishop1;
	if(distance%2==0){
		//this is bad, either both odd or both even
		return false;
	}else if(distance%2==1){
		return true;
	}else{
		std::cout << "ERROR CHECKING BISHOPS\n";
		return false;
	}
}

//Set board up here.
void PickPieces(bool mirrored)
{
	srand(time(NULL));
	rand();	//this line is odd, but the rand function seems to need 1 call before giving truly random numbers.

	while(true){
		GenRow(whtPieces);
		if(CheckRow(whtPieces))
			break;
	}
	if(!mirrored){
		while(true){
			GenRow(blkPieces);
			if(CheckRow(blkPieces))
				break;
		}
	}else if (mirrored){
		for(int i=0; i<8; i++){
			blkPieces[i]=whtPieces[i];		//maybe go from 7-0?
		}
	}
}


