#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pdftotxt.h"
#define OUTPUT_BUFFER 10
int main(int argc, char *argv[])
{
  if(argc<=1)//checking if there are no arguments entered with the command
  {
    printf("You have not entered any files to be converted\n");
    printf("Please 2 arguments\npdf file to be converted and text file to which you want to convert it to\n");
  }
  else if(argc>3)//checking if there are too many arguments passed in the command
  {
    printf("You have entered too many files in the command\n");
    printf("Please 2 arguments\npdf file to be converted and text file to which you want to convert it to\n");
  }
  else//code to run if the correct arguments are passed in the command
  {
    FILE * fp;//declaring a file pointer
    char *buf_in, *buf_out;//buf_in->to read the pdf text; buf_out->to get the outputed text from the pdf
    long len_in;
    size_t res, len_out = 0;

    fp = fopen(argv[1], "rb");//opening the pdf in binary read mode
    if (fp == NULL)//check if the pdf name is written properly
    {
      printf("Kindly enter a proper pdf name with .pdf at the end\n");
      return 0;
    }

    fseek(fp, 0, SEEK_END);//seek the file pointer to the end
    len_in = ftell(fp);//get the file pointer location which indirectly gives the size of the file
    rewind(fp);//get the file pointer back to the starting

    buf_in = (char *)malloc(sizeof(char) * len_in);//initialize buf_in to a string of size len_in
    if (buf_in == NULL)
    {
      return -1;
    }

    res = fread(buf_in, 1, len_in, fp);//read the file pointer fp and store the read data in buf_in
    if (res != (size_t)len_in)
    {
      return -1;
    }

    buf_out = (char *)malloc(sizeof(char) * res * OUTPUT_BUFFER);//initializing buf_out
    if (buf_out == NULL)
    {
      return -1;
    }

    pdftotxt(buf_out, (size_t *)&len_out, buf_in, res);//calling pdftotxt function
    /*
    printf("extraction waala: %lu characters\n\n", len_out);
    printf("output:\n%s\n", buf_out);
    */
    fclose(fp);

    //writing the retrived data that has been decrypted to a text file
    fp = fopen(argv[2], "w");//open the desired text file in write mode
    if (fp == NULL)//check if the text file name is written properly
    {
      printf("Kindly enter a proper text file name with .txt at the end\n");
      return 0;
    }
    fputs(buf_out,fp);//write the contents of buf_out to the file pointer
    fclose (fp);

    //freeing up the initialzed space used in malloc functions
    free(buf_in);
    free(buf_out);
  }


  return 0;
}
