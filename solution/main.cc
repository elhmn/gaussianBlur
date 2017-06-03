#include <iostream>
#include <cerrno>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define TAB			"\t"

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

typedef enum		e_compress
{
	BI_RGB = 0,
	BI_RLE8,
	BI_RLE4,
	BI_BITFIELDS,
	BI_JPEG,
	BI_PNG,
}					t_e_compress;

typedef struct		s_bmp_header
{
	unsigned char	type[2];
	uint32_t		file_sz;
	uint16_t		creator1;
	uint16_t		creator2;
	uint32_t		offset;

/*
** non member functions
*/
	void			dump(void)
	{
		std::cout << "bmp_header : " << std::endl \
		<< TAB << "type : " << this->type << std::endl \
		<< TAB << "file_sz : " << this->file_sz << std::endl \
		<< TAB << "creator1 : " << this->creator1 << std::endl \
		<< TAB << "creator2 : " << this->creator2 << std::endl \
		<< TAB << "offset : " << this->offset << std::endl \
		<< "END" << std::endl;
	};
}					t_bmp_header;

/*
** The structure bellow is only relevant
** for Windows V3 DIB info header
*/

typedef struct		s_bmpinfo_header
{
	uint32_t		header_sz;
	int32_t			width;
	int32_t			height;
	uint16_t		nplanes;
	uint16_t		color_depth;
	uint32_t		compress_t;
	uint32_t		image_sz;
	uint32_t		hres;
	uint32_t		vres;
	uint32_t		n_colors;
	uint32_t		n_imp_colors;

/*
** non member functions
*/
	void			dump(void)
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
}					t_bmpinfo_header;



static void			show_usage(void)
{
	std::cout << "usage : gbfilter input_file output_file kernel_size" \
				<< " tile_width tile_height" << std::endl;
}

static void			error(const char *msg = 0)
{
	std::cout << "Error : " << __FILE__ << " : " << __LINE__;
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

void				init_struct(t_bmp_header **bmp_header,
								t_bmpinfo_header **bmpinfo)
{
	if (!(*bmp_header = new s_bmp_header()))
		error("BAD ALLOC");
	if (!(*bmpinfo = new s_bmpinfo_header()))
		error("BAD ALLOC");
}

void				destroy_struct(t_bmp_header **bmp_header,
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

void				set_structure(unsigned char *buf, t_bmp_header *bmp_header,
									t_bmpinfo_header *bmpinfo)
{
	if (!bmp_header || !bmpinfo)
		error("Empty pointer");

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
	//if () header != 40 warning the blabla type is not correct it may cause the program to fail
	bmpinfo->width = *(int32_t*)(buf + 18);
	bmpinfo->height = *(int32_t*)(buf + 22);
	bmpinfo->nplanes = *(uint16_t*)(buf + 26);
	bmpinfo->color_depth = *(uint16_t*)(buf + 28);
	if (bmpinfo->color_depth != 24)
		error("Progam only handle 24pbb files");
	bmpinfo->compress_t = *(uint32_t*)(buf + 30);
	if (bmpinfo->compress_t != BI_RGB)
		error("Program does'nt not handle compressed files");
	bmpinfo->image_sz = *(uint32_t*)(buf + 34);
	bmpinfo->hres = *(uint32_t*)(buf + 38);
	bmpinfo->vres = *(uint32_t*)(buf + 42);
	bmpinfo->n_colors = *(uint32_t*)(buf + 46);
	bmpinfo->n_imp_colors = *(uint32_t*)(buf + 50);

#ifdef DEBUG
	bmpinfo->dump();//_DEBUG_//
#endif
}

void				readFile(char *filePath)
{
	int					fd;
	unsigned char		buf[54];
	t_bmp_header		*bmp_header;
	t_bmpinfo_header	*bmpinfo;

	fd = 42;
	if (!filePath)
		error("filePath set to NULL");
	if ((fd = open(filePath, O_RDONLY)) < 0)
	{
		perror(filePath);
		error();
	}
	init_struct(&bmp_header, &bmpinfo);
	if (read(fd, (void*)buf, sizeof(buf)) < 0)
	{
		perror("filePath while reading");
		error();
	}
	set_structure(buf, bmp_header, bmpinfo);
	//read file data
	destroy_struct(&bmp_header, &bmpinfo);
}

int					main(int ac, char **av)
{
	if (ac != 6)
		show_usage();
	else
	{
		if (!av)
			error();
		readFile(av[1]);
	}
	return (0);
}
