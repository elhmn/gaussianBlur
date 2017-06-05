#include <iostream>
#include <cerrno>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <fcntl.h>
#include <cmath>
#include <iomanip>

#define TAB			"\t"
#define PRINT_W	40
#define ERROR(x)			error((x), __FILE__, __LINE__)

# define LIMIT_SCOL(x)	(x > 1 || x < 0) ? ((x > 1) ? 1 : 0) : x
# define LIMIT_COL(x)	(x > 255. || x < 0.) ? ((x > 255.) ? 255. : 0.) : x

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

/*
** Filter variables
*/
	float				**weightMap;
	float				weightSum;
	float				filterN;

/*
** tiles
*/
	int					tile_w;
	int					tile_h;

	int					junkBytes;
	size_t				headerSize;
	t_bmp_header		*bmp_header;
	t_bmpinfo_header	*bmpinfo;
}						t_env;

void					proc_start(const char *x, const char *y = " ")
{
	std::cout << std::setw(PRINT_W) << std::left << std::string(x) + " " + y \
				<< std::setw(PRINT_W) << std::right << "..." << std::endl;
}

void					proc_ok(const char *x, const char *y = " ")
{
	std::cout << std::setw(PRINT_W) << std::left << std::string(x) + " " + y \
				<< std::setw(PRINT_W) << std::right << "OK" << std::endl;
}

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
** OPTIMIZATION
** 1- set filter in its own tab
** 2- limit gaussian inside the image bounderies
** Check that input_file has the bmp extension (maybe is not relevant)
** Check that the input_file exists
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
	if (env[0]->weightMap)
	{
		for (int i = 0; i < env[0]->filterN; i++)
			free(env[0]->weightMap[i]);
		free(env[0]->weightMap);
		env[0]->weightMap = NULL;
	}
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

/*
**ifdef DEBUG
*/
	bmp_header->dump();
/*
**#endif
*/

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

/*
**ifdef DEBUG
*/
	bmpinfo->dump();
/*
**#endif
*/
}

void					readFile(t_env *env, const char *filePath)
{
	int					fd;
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
	proc_start("Reading file", filePath);
	if (read(fd, (void*)env->headerData, bufSize) < 0)
	{
		perror("filePath while reading");
		ERROR("");
	}
	set_structure(env->headerData, env->bmp_header, env->bmpinfo);
	env->junkBytes = 4 - ((env->bmpinfo->width * 3) % 4);
 	bufSize = (env->bmpinfo->width * 3 + env->junkBytes) * env->bmpinfo->height;
	env->imgSize = bufSize;
	if (!(env->imgData = new unsigned char[bufSize]))
		ERROR("BAD ALLOC");
	if (pread(fd, (void*)env->imgData, bufSize, env->bmp_header->offset) < 0)
	{
		perror("filePath while reading");
		ERROR("");
	}
	close(fd);
	proc_ok("Reading file", filePath);
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

/*
** #ifdef DEBUG
*/
	std::cout << "BLUR PARAM :" << std::endl;
	std::cout << TAB << "kernel size = " << *kSize << std::endl;
	std::cout << TAB << "tile width = " << *tw << std::endl;
	std::cout << TAB << "tile height = " << *th << std::endl;
	std::cout << "END" << std::endl;
/*
** #endif
*/
}

void					pixel_put(t_env *env,
							int x, int y, uint8_t r,
							uint8_t g, uint8_t b)
{
	int			data_w;

	if (!env)
		ERROR("env set to NULL");
	if (!env->bmpinfo)
		ERROR("env->bmpinfo set to NULL");
	data_w = env->bmpinfo->width * 3 + env->junkBytes;
	y = env->bmpinfo->height - y - 1;
	if (env->imgData && x >= 0 && x < env->bmpinfo->width
			&& y >= 0 && y < env->bmpinfo->height)
	{
		env->imgData[y * data_w + (x * 3)] = (uint8_t)LIMIT_COL(b);
		env->imgData[y * data_w + (x * 3) + 1] = (uint8_t)LIMIT_COL(g);
		env->imgData[y * data_w + (x * 3) + 2] = (uint8_t)LIMIT_COL(r);
	}
}

void					getColor(t_env *env,
							int x, int y, uint8_t *r,
							uint8_t *g, uint8_t *b)
{
	int			data_w;

	if (!env)
		ERROR("env set to NULL");
	if (!env->bmpinfo)
		ERROR("env->bmpinfo set to NULL");
	if (!r || !g || !b)
		ERROR("r or g or b set to NULL");
	data_w = env->bmpinfo->width * 3 + env->junkBytes;
	y = env->bmpinfo->height - y - 1;
	if (env->imgData && x >= 0 && x < env->bmpinfo->width
			&& y >= 0 && y < env->bmpinfo->height)
	{
		*b = (uint8_t)env->imgData[y * data_w + (x * 3)];
		*g = (uint8_t)env->imgData[y * data_w + (x * 3) + 1];
		*r = (uint8_t)env->imgData[y * data_w + (x * 3) + 2];
	}
}

static float			weight(int x, int y, float kSize)
{
	float	kSquare;
	float	a;

	kSquare = 2. * pow(kSize, 2);
	a = pow((float)x, 2) + pow((float)y, 2) / kSquare;
	return (exp(-a));
}

/*
** x_i && y_i are only used for optimization
*/
void					blur(t_env *env, int x, int y, int x_i, int y_i)
{
	uint8_t		r;
	uint8_t		g;
	uint8_t		b;

/*
** new color values
*/
	float		n_r;
	float		n_g;
	float		n_b;

	int			i;
	int			j;
	int			n;

	float		s;
	float		wTmp;

	if (env->imgData && x >= 0 && x < env->bmpinfo->width
			&& y >= 0 && y < env->bmpinfo->height)
	{
		s = env->weightSum;
		n_r = 0;
		n_g = 0;
		n_b = 0;
		n = env->filterN;
		i = -n - 1;
		while (++i < n)
		{
			j = -n - 1;
			while (++j < n)
			{
				if (x_i + j < 0 || x_i + j >= env->tile_w
						|| y_i + i < 0
						|| y_i + i >= env->tile_h)
					continue ;
				getColor(env, x + j, y + i, &r, &g, &b);
				wTmp = env->weightMap[j + n][i + n];
				n_r += wTmp * r;
				n_g += wTmp * g;
				n_b += wTmp * b;
			}
		}
		n_r /= s;
		n_g /= s;
		n_b /= s;
 		pixel_put(env, x, y, LIMIT_COL(n_r),
 					LIMIT_COL(n_g), LIMIT_COL(n_b));
	}
}

void					setFilter(t_env *env, float kSize)
{
	int			i;
	int			j;
	int			n;
	int			tmp;

	if (!env)
		ERROR("env set to NULL");
	proc_start("Setting filter");
/*
** n = kSize * (2...3) to avoid clipping
*/
	env->filterN = kSize * 2;
	n = env->filterN;
	if (!(env->weightMap = (float**)malloc(sizeof(float*) * (n * 2 + 1))))
		ERROR("env->weightMap BAD ALLOC");
	tmp = -1;
	while (++tmp < 2 * n)
	{
		if (!(env->weightMap[tmp]
				= (float*)malloc(sizeof(float) * (n * 2 + 1))))
			ERROR("env->weightMap BAD ALLOC");
	}
	i = -n - 1;
	env->weightSum = 0;
	while (++i < n)
	{
		j = -n - 1;
		while (++j < n)
		{
			env->weightMap[j + n][i + n] = weight(j, i, kSize);
			env->weightSum += env->weightMap[j + n][i + n];
		}
	}
	proc_ok("Setting filter");
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

	if (!env)
		ERROR("env set to NULL");
	parse(env, &kSize, &tw, &th, kernel_sz, tile_w, tile_h);
	setFilter(env, kSize);
	width = env->bmpinfo->width;
	height = env->bmpinfo->height;
	env->tile_w = tw;
	env->tile_h = th;
	tileCount_w = (width / tw) + 1;
	tileCount_h = (height / th) + 1;
	tileCount_y = -1;
	while (++tileCount_y < tileCount_h)
	{
		tileCount_x = -1;
		while (++tileCount_x < tileCount_w)
		{
			proc_start("Blur", (std::string("tile [").c_str()
						+ std::to_string(tileCount_x) + "]["
						+ std::to_string(tileCount_y) + "]").c_str());
			y = -1;
			while (++y < th)
			{
				x = -1;
				while (++x < tw)
				{
					blur(env, tileCount_x * tw + x,
							tileCount_y * th + y, x, y);
#ifdef DEBUG
 					if (x == 0 || y == 0)
 						pixel_put(env, tileCount_x * tw + x,
 							tileCount_y * th + y, 0x00, 0x00, 0xFF);
#endif
				}
			}
			proc_ok("Blur", (std::string("tile [").c_str()
						+ std::to_string(tileCount_x) + "]["
						+ std::to_string(tileCount_y) + "]").c_str());
		}
	}
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
	proc_start("Writing file", filePath);
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
	proc_ok("Writing file", filePath);
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
