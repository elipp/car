#include <Winsock2.h>
#include <Windows.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "vertex.h"
#include "objloader.h"
#include "common.h"


using namespace std;

static int VertexVBOid;

static const char* extstr[3] = { ".obj", ".bobj", ".cbobj" };


int loadObj(const char *filename)
{
	return 0;
}


/**
 *	Loads a binary .obj file (.bobj or even .cbobj for qlz-compressed) from file filename.
 *	This should be really ez.
 */

unsigned short int* loadBObj(const char* filename, bool compressed, GLuint *VBOid, GLuint *facecount)
{
	
	/*
	 * Note: a lot of plain old C-functionality has been commented out, and replaced with C++ equivalents
	 * (FILE pointers replaced with std::ifstream objects, size_t with std::size_t etc)
	 */

	// check if file extension matches with the specified options 
	
	char *buffer = NULL;
	//FILE *infile = fopen(filename, "rb");
	std::ifstream infile(filename, std::ios::in | std::ios::binary);
	std::size_t filesize;

	//if (!infile)
	if(!infile.is_open())
	{
		logWindowOutput( "Couldn't open input file %s. Aborting.\n", filename);
		return NULL;
	}

	if (compressed)
	{
		if (!checkext(filename, CBOBJ)){ return NULL; }	
		logWindowOutput( "input file is compressed. decompressing\n");
		//decompress_qlz(infile, &buffer);
	}
	else {
		
		if (!checkext(filename, BOBJ)) { return NULL; }	
		filesize = cpp_getfilesize(infile);

		buffer = new char[filesize];
		
		infile.read(buffer, filesize);
	}

	//fclose(infile);
	infile.close();	

		
	// check file header for "bobj"

	if (!headerValid(buffer))
		return NULL; 

	// load count info into an int[5] by just memsetting.
		
		char* iter = buffer+sizeof(int);	// the first 4 bytes are 0x6a626f62 for "bobj".
		long curoffset = 0;					// keep track of current offset
		int info[5];

		memcpy(info, iter, 5*sizeof(int));	// copy count information 

		*facecount = info[4];	// info[4] = triangle face count

		// start memcpying directly to a float array of size info[BVC] 

		// go to offset 6*sizeof(int)
		GOTO_OFFSET_SUM(buffer, iter, 6*sizeof(int), curoffset);

		// copy a total of 3*info[BVC] floats directly (each vertex entry contains 3 floats)
		float *vertices = new float[3*info[BVC]];
		memcpy(vertices, iter, 3*info[BVC]*sizeof(float));

		/*for(int i = 0; i < 3*info[BVC]; )
		{
			printf("%f %f %f\n", vertices[i], vertices[i+1], vertices[i+2]);
			i += 3;
		} */

		// copy a total of 3*info[NVC] floats directly (each normal entry contains 3 floats as well)
		GOTO_OFFSET_SUM(buffer, iter, 3*info[BVC]*sizeof(float), curoffset);
		
		float *normals = new float[3*info[BNC]];
		memcpy(normals, iter, 3*info[BNC]*sizeof(float));

		
		/*for(int i = 0; i < 3*info[BNC]; )
		{
			printf("%f %f %f\n", normals[i], normals[i+1], normals[i+2]);
			i += 3;
		} */

		// TODO: COPY TEXCOORDS HERE <INSERT>


		
		
		size_t elem_size = (info[BVC] < 0xffff ? sizeof(unsigned short int) : sizeof(unsigned int));

		
		// copy qfaces. If there are less than 65535 vertices in the file, the file format uses short uints for storage.
		GOTO_OFFSET_SUM(buffer, iter, 3*info[BNC]*sizeof(float), curoffset);
		void *vqfaces = malloc(4 * elem_size * info[BQFC]);	// NOTE! the following '4' modifiers used to be 8's: QF-entries used to have 8 integers in them.
		
		memcpy(vqfaces, iter, 4 * elem_size * info[BQFC]); 
		
		unsigned short int *qfaces = (unsigned short int*) vqfaces;	// cast for easy manipulation (indexing etc). 
			

		// TODO: handle the case of info[BVC] > 0xffff ==> unsigned ints 
		
	
		// copy tfaces.

		GOTO_OFFSET_SUM(buffer,iter, 4 * elem_size * info[BQFC], curoffset);	// 8 CHANGED TO 4 (MATCHED NORMALS)
		void *vtfaces = malloc(3 * elem_size * info[BTFC]);		// only 3 integers on each line (CHANGED FROM 6)
		memcpy(vtfaces, iter, 3 * elem_size * info[BTFC]);
		unsigned short int *tfaces = (unsigned short int*) vtfaces;
	

		delete [] buffer; // not needed hereafter.

		/**
		 * 
		 * Convert all this data into a more useful format (struct vertex[]).
		 *
		 */
		
		vertex *vertarray = new vertex[info[BVC]];
		
		int i=0;	// deliberately declaring int i outside of the loop
		for (; i < info[BVC]; i++)
		{
			vertarray[i].vx = vertices[3*i];
			vertarray[i].vy = vertices[3*i + 1];
			vertarray[i].vz = vertices[3*i + 2];

			vertarray[i].nx = normals[3*i];	// they are matched these days =)
			vertarray[i].ny = normals[3*i + 1];
			vertarray[i].nz = normals[3*i + 2]; 

		
			/*printf("[%d] %f %f %f // %f %f %f\n", i, 
												vertarray[i].vx, vertarray[i].vy, vertarray[i].vz,
												vertarray[i].nx, vertarray[i].ny, vertarray[i].nz); */
		
		  
		}

		delete [] vertices;
		delete [] normals;
		
		// 

		/* Create VBO for vertex data */
		
		glGenBuffers(1, VBOid);
		glBindBuffer(GL_ARRAY_BUFFER, *VBOid);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*info[BVC], &vertarray[0].vx, GL_STATIC_DRAW);


		free(vqfaces);

		//return qfaces;
		return tfaces;
}



bool checkext(const char* filename, int extID)
{

		size_t filename_len = strlen(filename);
		size_t extstr_len = strlen(extstr[extID]);
		
		
		unsigned int j = extstr_len - 1;
		
		for (unsigned int i = filename_len-1; i >= filename_len - extstr_len; i--)
		{			
			if (filename[i] != extstr[extID][j])
			{
				logWindowOutput( "The model file you're trying to load doesn't have extension %s. Aborting!\n", extstr[extID]);
				return false;
			}
			
			j--;
		}
		return true;

}

bool headerValid(char* buffer)
{
	return (*((int*)buffer) == bobj ? true : false);	// "bobj" == 0x6a626f62;
}

/*

size_t decompress(FILE *file, char** buffer)
{

	// uncompress file with quicklz.

	qlz_state_decompress *state = new qlz_state_decompress;
	
	size_t infile_size = getfilesize(file);
	char* compressed_buf = new char[infile_size];
	fread(compressed_buf, 1, infile_size, file);

	// read decompressed buffer size pre-emptively
	size_t uncompressed_size = qlz_size_decompressed(compressed_buf);

	*buffer = new char[uncompressed_size];	

	size_t filesize = qlz_decompress(compressed_buf, *buffer, state);
		
	delete [] compressed_buf;		// these are not needed anymore
	delete state;

	return filesize;
}

*/


GLuint loadNewestBObj(const std::string &filename, GLuint *facecount) {

	std::ifstream infile(filename.c_str(), std::ios::binary | std::ios::in);
	
	if (!infile.is_open()) { logWindowOutput( "loadNewestBObj: failed loading file %s\n", filename.c_str()); return NULL; }
	
	logWindowOutput("loadNewestBObj: loading file %s\n", filename.c_str());

	std::size_t filesize = cpp_getfilesize(infile);
	char *buffer = new char[filesize];

	infile.read(buffer, filesize);
	
	char* iter = buffer + 4;	// the first 4 bytes of a bobj file contain the letters "bobj". Or do they? :D

	// read vertex & face count.

	unsigned int vcount = *((unsigned int*)iter);
	*facecount = (GLuint)(vcount/3);

	iter += 4;

	vertex* vertices = new vertex[vcount];

	iter = buffer + 8;

	memcpy(vertices, iter, vcount*8*sizeof(float));

	GLuint VBOid;

	glGenBuffers(1, &VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vcount, &vertices[0].vx, GL_STATIC_DRAW);

	//printf("%f %f %f", vertices[0].vx, vertices[0].vy, vertices[0].vz);

	return VBOid;


}
