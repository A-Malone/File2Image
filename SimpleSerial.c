#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

//----COMMAND TO COMPILE
//gcc -std=c99 CreateImage.c -lm -lpng

//Function declarations
png_bytep * read_png_file(char* file_name, int * w, int* h, int*);
int readFromPNG(char * inName, char * outName);
int writeToPNG(char * inName, char * outName);



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//-----------------------------------WRITING TO FILE-------------------------------------------
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
    
/* A coloured pixel.
 * Colour depth is 16
 */

typedef struct {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} pixel_t;

/* A picture. */    
typedef struct  {
    pixel_t *pixels;
    size_t width;
    size_t height;
} bitmap_t;
    
/* Given "bitmap", this returns the pixel of bitmap at the point 
   ("x", "y"). */

static pixel_t * pixel_at (bitmap_t * bitmap, int x, int y)
{
    return bitmap->pixels + bitmap->width * y + x;
}
    
/* Write "bitmap" to a PNG file specified by "path"; returns 0 on
   success, non-zero on error. */

static int save_png_to_file (bitmap_t *bitmap, const char *path)
{
    FILE * fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_byte ** row_pointers = NULL;
    /* "status" contains the return value of this function. At first
       it is set to a value which means 'failure'. When the routine
       has finished its work, it is set to a value which means
       'success'. */
    int status = -1;
    /* The following number is set by trial and error only. I cannot
       see where it it is documented in the libpng manual.
    */
    int pixel_size = 3;
    int depth = 16;
    
    fp = fopen (path, "wb");
    if (! fp) {
        goto fopen_failed;
    }

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        goto png_create_write_struct_failed;
    }
    
    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        goto png_create_info_struct_failed;
    }
    
    /* Set up error handling. */

    if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure;
    }
    
    /* Set image attributes. */

    png_set_IHDR (png_ptr,
                  info_ptr,
                  bitmap->width,
                  bitmap->height,
                  depth,
                  PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);
    
    /* Initialize rows of PNG. */

    int depth_factor = depth/8;   
    row_pointers = png_malloc (png_ptr, bitmap->height * sizeof (png_byte *));
    for (y = 0; y < bitmap->height; ++y) {
        png_byte *row = 
            png_malloc (png_ptr, sizeof (uint16_t) * bitmap->width * pixel_size);
        row_pointers[y] = row;
        for (x = 0; x < bitmap->width; ++x) {
            pixel_t * pixel = pixel_at (bitmap, x, y);

            *row = pixel->red;
            row+=depth_factor;

            *row = pixel->green;
            row+=depth_factor;

            *row = pixel->blue;
            row+=depth_factor;
        }
    }
    
    /* Write the image data to "fp". */

    png_init_io (png_ptr, fp);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    /* The routine has successfully written the file, so we set
       "status" to a value which indicates success. */

    status = 0;
    
    for (y = 0; y < bitmap->height; y++) {
        png_free (png_ptr, row_pointers[y]);
    }
    png_free (png_ptr, row_pointers);
    
 png_failure:
 png_create_info_struct_failed:
    png_destroy_write_struct (&png_ptr, &info_ptr);
 png_create_write_struct_failed:
    fclose (fp);
 fopen_failed:
    return status;
}

/* Given "value" and "max", the maximum value which we expect "value"
   to take, this returns an integer between 0 and 255 proportional to
   "value" divided by "max". */

static int pix (int value, int max)
{
    if (value < 0)
        return 0;
    return (int) (256.0 *((double) (value)/(double) max));
}

int set_color(FILE* in_file, long num_chars, pixel_t * pixel){
    char num = 0; 
    char data[3];
    int status = 1;
    //Gets the data from the file
    for(num = 0; num<3 && ftell(in_file) < num_chars; ++num){
        fread(&(data[num]),sizeof(char),1,in_file);
    }

    //Set the rest that don't get read to 0
    for (;num<3; ++num){
        data[num] = 0;
        status = 0;
    }
   
    (*pixel).red   = (int)(data[0]);
    (*pixel).green = (int)(data[1]);
    (*pixel).blue  = (int)(data[2]);

    return status;
}

int writeToPNG(char * inName, char * outName){
    bitmap_t map;
    int x = 0;
    int y = 0;
    
    FILE* in_file = fopen(inName, "rb");
    
    //Gets the length of the file in chars
    fseek(in_file, 0L, SEEK_END);
    long num_chars = ftell(in_file);
    fseek(in_file, 0L, SEEK_SET);

    int dim = ceil(sqrt(num_chars/3));

    int status = 1;   

    /* Create an image. */

    map.width = dim;
    map.height = dim;

    map.pixels = calloc (sizeof (pixel_t), map.width * map.height);

    for (y = 0; y < map.height; y++) {
        for (x = 0; x < map.width; x++) {
            if (status){
                pixel_t * pixel = pixel_at(& map, x, y);
                status = set_color(in_file, num_chars, pixel);                
            }
            else
            {
                pixel_t * pixel = pixel_at(& map, x, y);
                (*pixel).red = 0;
                (*pixel).green = 0;
                (*pixel).blue = 0;
                //fprintf(stderr, "%d ", y);
            }
        }
    }
    
    fclose(in_file);

    /* Write the image to a file 'map.png'. */
    save_png_to_file (& map, outName);

    //Free the uneeded pixels
    free(map.pixels);
}

int writeOverlayedPNG(char * inName, char * outName, char * maskName){
    bitmap_t map;
    int x = 0;
    int y = 0;
    
    FILE* in_file = fopen(inName, "rb");

    
    //Gets the length of the file in chars
    fseek(in_file, 0L, SEEK_END);
    long num_chars = ftell(in_file);
    fseek(in_file, 0L, SEEK_SET);

    //Get the dimensions of the mask
    int width,height, depth_factor;
    png_bytep * mask_row_pointers = read_png_file(maskName,&width,&height, &depth_factor);
    if(num_chars/3 > width*height){
        fprintf(stderr, "%s\n", "File too large for mask");
        return -1;
    }

    int status = 1;   

    /* Create an image. */

    map.width = width;
    map.height = height;

    map.pixels = calloc (sizeof (pixel_t), map.width * map.height);

    for (y = 0; y < map.height; y++) {
        for (x = 0; x < map.width; x++) {
            if (status){
                //Get the pixel
                pixel_t * pixel = pixel_at(& map, x, y);

                //Set the next pixel with the next characters
                status = set_color(in_file, num_chars, pixel);                
            }
            else
            {
                pixel_t * pixel = pixel_at(& map, x, y);
                (*pixel).red = 0;
                (*pixel).green = 0;
                (*pixel).blue = 0;
                //fprintf(stderr, "%d ", y);
            }
        }
    }
    
    fclose(in_file);

    /* Write the image to a file 'map.png'. */
    save_png_to_file (& map, outName);
    free(map.pixels);
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//-----------------------------------READING FROM FILE-----------------------------------------
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

png_bytep * read_png_file(char* file_name, int * w, int* h, int* depth_factor)
{
    int x, y;

    int width, height;
    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    png_bytep * row_pointers;
    char header[8];    // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(file_name, "rb");
    if (!fp)
            abort_("[read_png_file] File %s could not be opened for reading", file_name);
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8))
            abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
            abort_("[read_png_file] png_create_read_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
            abort_("[read_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);      //Get the signature bytes

    png_read_info(png_ptr, info_ptr);

    //Get the info of the png
    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);;
    *w = width;
    *h = height;

    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    fprintf(stderr, "Bit depth is %d\n", (int)bit_depth);
    *depth_factor = bit_depth/8;

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);


    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[read_png_file] Error during read_image");

    row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (y=0; y<height; y++)
            row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

    png_read_image(png_ptr, row_pointers);
    return(row_pointers);

    fclose(fp);
    free(png_ptr);
}

int readFromPNG(char * inName, char * outName){
    FILE * outfile = fopen(outName, "wb");
    int width,height, depth_factor;
    png_bytep * row_pointers = read_png_file(inName,&width,&height, &depth_factor);
    for (int y = 0; y < height; y++) {
        png_byte* row = row_pointers[y];
        for (int x = 0; x < width; x++) {

            png_byte* ptr = &(row[x*3*depth_factor]);

            //Write the picture back out (currently writes low-bit)
            for (int i = 0; i < 3; ++i)
            {
                fwrite(ptr+i*depth_factor, sizeof(char), 1, outfile);
            }
            
        }
        free(row);
        row_pointers[y] = NULL;
    }
    free(row_pointers);
    fclose(outfile);
}



int main ()
{   
    //Serializing and deserializing complete
    writeToPNG("blah.doc", "map.png");
    readFromPNG("map.png", "out.doc");

    return 0;
}

