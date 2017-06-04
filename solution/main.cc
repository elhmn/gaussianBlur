#include <iostream>
#include <cerrno>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define TAB			"\t"
#define ERROR(x)	error((x), __FILE__, __LINE__)

/*
** This constants are set by the compiler for test purpose
**
**
** #define DEBUG
**
**
*/


/*
** The t_e_compress enum will be used to certify
** that there is no compression in the bmp because
** i won't take any compression into consideration
*/

typedef enum			e_compress
{
	BI_RGB = 0,
	BI_RLE8,
	BI_RLE4,
	BI_BITFIELDS,
	BI_JPEG,
	BI_PNG,
}						t_e_compress;

typedef struct			s_bmp_header
{
	unsigned char		type[2];
	uint32_t			file_sz;
	uint16_t			creator1;
	uint16_t			creator2;
	uint32_t			offset;

/*
** non member functions
*/
	void				dump(void)
	{
		std::cout << "bmp_header : " << std::endl \
		<< TAB << "type : " << this->type << std::endl \
		<< TAB << "file_sz : " << this->file_sz << std::endl \
		<< TAB << "creator1 : " << this->creator1 << std::endl \
		<< TAB << "creator2 : " << this->creator2 << std::endl \
		<< TAB << "offset : " << this->offset << std::endl \
		<< "END" << std::endl;
	};
}						t_bmp_header;

/*
** The structure bellow is only relevant
** for Windows V3 DIB info header
*/

typedef struct			s_bmpinfo_header
{
	uint32_t			header_sz;
	int32_t				width;
	int32_t				height;
	uint16_t			nplanes;
	uint16_t			color_depth;
	uint32_t			compress_t;
	uint32_t			image_sz;
	uint32_t			hres;
	uint32_t			vres;
	uint32_t			n_colors;
	uint32_t			n_imp_colors;

/*
** non member functions
*/
	void				dump(void)
	{
		std::cout << "bmp_info_header : " << std::endl \
		<< TAB << "header_sz : " << this->header_sz << std::endl \
		<< TAB << "height : " << this->height << std::endl \
		<< TAB << "width : " << this->width << std::endl \
		<< TAB << "nplanes : " << this->nplanes << std::endl \
		<< TAB << "color_depth : " << this->color_depth << std::endl \
		<< TAB << "compress_t : " << this->compress_t << std::endl \
		<< TAB << "image_sz : " << this->image_sz << std::endl \
		<< TAB << "hres : " << this->hres << std::endl \
		<< TAB << "vres : " << this->vres << std::endl \
		<< TAB << "n_colors : " << this->n_colors << std::endl \
		<< TAB << "n_imp_colors : " << this->n_imp_colors << std::endl \
		<< "END" << std::endl;
	}
}						t_bmpinfo_header;

/*
** program env
*/

typedef struct			s_env
{
	unsigned char		headerData[54];
	unsigned char		*imgData;
	size_t				imgSize;
	size_t				headerSize;
	t_bmp_header		*bmp_header;
	t_bmpinfo_header	*bmpinfo;
}						t_env;

static void				show_usage(void)
{
	std::cout << "usage : gbfilter input_file output_file kernel_size" \
				<< " tile_width tile_height" << std::endl;
}

static void				error(const char *msg = 0,
								const char *file = 0, int line = 0)
{
	std::cout << "Error : " << file << " : " << line;
	if (msg)
		std::cout << " : " << msg;
	std::cout << std::endl;
	exit(-1);
}

/*
** TODO
** Check parameters count
** Check that input_file has the bmp extension (maybe is not relevant)
** Check that the input_file exists
**  Read BMP file
*/

void					init_struct(t_bmp_header **bmp_header,
									t_bmpinfo_header **bmpinfo)
{
	if (!(*bmp_header = new s_bmp_header()))
		ERROR("BAD ALLOC");
	if (!(*bmpinfo = new s_bmpinfo_header()))
		ERROR("BAD ALLOC");
}

void					destroy_struct(t_bmp_header **bmp_header,
									t_bmpinfo_header **bmpinfo)
{
	if (bmp_header)
	{
		delete (*bmp_header);
		*bmp_header = NULL;
	}
	if (bmpinfo)
	{
		delete (*bmpinfo);
		*bmpinfo = NULL;
	}
}

void					destroy(t_env **env)
{
	if (!env || !*env)
		ERROR("env or *env set to NULL");
	destroy_struct(&env[0]->bmp_header, &env[0]->bmpinfo);
	if (env[0]->imgData)
		delete env[0]->imgData;
	env[0]->imgData = NULL;
	*env = NULL;
}

void					set_structure(unsigned char *buf,
									t_bmp_header *bmp_header,
									t_bmpinfo_header *bmpinfo)
{
	if (!bmp_header || !bmpinfo)
		ERROR("Empty pointer");

/*
** BMP header data parsing
*/
	memcpy(bmp_header->type, buf, 2);
	bmp_header->file_sz = *(uint32_t*)(buf + 2);
	bmp_header->creator1 = *(uint16_t*)(buf + 6);
	bmp_header->creator2 = *(uint16_t*)(buf + 8);
	bmp_header->offset = *(uint32_t*)(buf + 10);

#ifdef DEBUG
	bmp_header->dump();//_DEBUG_//
#endif

/*
** BMP header info data parsing
*/
	bmpinfo->header_sz = buf[14];
	if (bmpinfo->header_sz != 40)
		ERROR("BMP image info header > 40");
	bmpinfo->width = *(int32_t*)(buf + 18);
	bmpinfo->height = *(int32_t*)(buf + 22);
	bmpinfo->nplanes = *(uint16_t*)(buf + 26);
	bmpinfo->color_depth = *(uint16_t*)(buf + 28);
	if (bmpinfo->color_depth != 24)
		ERROR("Progam only handle 24pbb files");
	bmpinfo->compress_t = *(uint32_t*)(buf + 30);
	if (bmpinfo->compress_t != BI_RGB)
		ERROR("Program does'nt not handle compressed files");
	bmpinfo->image_sz = *(uint32_t*)(buf + 34);
	bmpinfo->hres = *(uint32_t*)(buf + 38);
	bmpinfo->vres = *(uint32_t*)(buf + 42);
	bmpinfo->n_colors = *(uint32_t*)(buf + 46);
	bmpinfo->n_imp_colors = *(uint32_t*)(buf + 50);

#ifdef DEBUG
	bmpinfo->dump();//_DEBUG_//
#endif
}

void					readFile(t_env *env, const char *filePath)
{
	int					fd;
	int					junkBytes;
	size_t				bufSize;

	fd = 42;
	if (!env)
		ERROR("env set NULL");
	bufSize = sizeof(env->headerData);
	if (!filePath)
		ERROR("filePath set to NULL");
	if ((fd = open(filePath, O_RDONLY)) < 0)
	{
		perror(filePath);
		ERROR("");
	}
	init_struct(&env->bmp_header, &env->bmpinfo);
	env->headerSize = bufSize;
	if (read(fd, (void*)env->headerData, bufSize) < 0)
	{
		perror("filePath while reading");
		ERROR("");
	}
	set_structure(env->headerData, env->bmp_header, env->bmpinfo);
	junkBytes = 4 - ((env->bmpinfo->width * 3) % 4);
 	bufSize = (env->bmpinfo->width * 3 + junkBytes) * env->bmpinfo->height;
	env->imgSize = bufSize;
	if (!(env->imgData = new unsigned char[bufSize]))
		ERROR("BAD ALLOC");
	if (pread(fd, (void*)env->imgData, bufSize, env->bmp_header->offset) < 0)
	{
		perror("filePath while reading");
		ERROR("");
	}
	close(fd);
}

static void				parse(t_env *env, float *kSize, int *tw, int *th,
							const char *kernel_sz, const char *tile_w,
							const char *tile_h)

{
	if (!env)
		ERROR("env set to NULL");
	if (!kSize || !tw || !th)
		ERROR("kSize or tw or th set to NULL");
	*kSize = atof(kernel_sz);
	*tw = atol(tile_w);
	*th = atol(tile_h);
	if (*tw > env->bmpinfo->width)
		ERROR("tile width can't be greater than image width");
	if (*tw < 0)
		ERROR("tile width can't be a negative value");
	if (*th > env->bmpinfo->height)
		ERROR("tile height can't be greater than image height");
	if (*th < 0)
		ERROR("tile height can't be a negative value");
	if (*kSize < 0)
		ERROR("tile kernelSize can't be a negative value");

#ifdef DEBUG
	std::cout << "BLUR PARAM :" << std::endl;
	std::cout << TAB << "kernel size = " << *kSize << std::endl;
	std::cout << TAB << "tile width = " << *tw << std::endl;
	std::cout << TAB << "tile height = " << *th << std::endl;
	std::cout << "END" << std::endl;
#endif
}

void					gaussianBlur(t_env *env, const char *kernel_sz,
							const char *tile_w, const char *tile_h)
{
	float				kSize;
	int					tw;
	int					th;

	int					tileCount_w;
	int					tileCount_h;
	int					tileCount_x;
	int					tileCount_y;

	int					x;
	int					y;

	int					width;
	int					height;
	unsigned char		*data;

	(void)x;
	(void)y;
	(void)data;
	if (!env)
		ERROR("env set to NULL");
	parse(env, &kSize, &tw, &th, kernel_sz, tile_w, tile_h);
	width = env->bmpinfo->width;
	height = env->bmpinfo->height;
	data = env->imgData;
	tileCount_w = (width / tw) + 1;
	tileCount_h = (height / th) + 1;
	tileCount_x = -1;
	while (++tileCount_x < tileCount_w)
	{
		tileCount_y = -1;
		while (++tileCount_y < tileCount_h)
		{

// 			std::cout << tileCount_x << ", " << tileCount_y << std::endl;//_DEBUG_//
		}
	}
// 	std::cout << "je suis gaussain blur" << std::endl;//_DEBUG_//
}

void					writeFile(t_env *env, const char *filePath)
{
	int					fd;

	fd = 42;
	if (!env)
		ERROR("env set to NULL");
	if (!filePath)
		ERROR("filePath set to NULL");
	if ((fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
	{
		perror(filePath);
		ERROR("");
	}
	if ((write(fd, env->headerData, env->headerSize)) < 0)
	{
		perror(filePath);
		ERROR("");
	}
	if ((pwrite(fd, env->imgData, env->imgSize,
			env->bmp_header->offset)) < 0)
	{
		perror(filePath);
		ERROR("");
	}
	close(fd);
}

int						main(int ac, char **av)
{
	t_env	*env;

	if (ac != 6)
		show_usage();
	else
	{
		if (!av)
			ERROR("");
		if (!(env = new t_env))
			ERROR("BAD ALLOC");
		readFile(env, av[1]);
		gaussianBlur(env, av[3], av[4], av[5]);
		writeFile(env, av[2]);
		destroy(&env);
	}
	return (0);
}
