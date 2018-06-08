#include <stdio.h> //includes input output
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "zlib.h"
#include "pdftotxt.h"
#define OUTPUT_BUFFER 10// Assume output will fit into 10 times input buffer
// function to find a string in a buffer
size_t findstringinbuffer
(char* buffer,
char* search,
size_t buffersize
)
{
  size_t i;
  char* buffer0 = buffer;//store the pdf text in a temporary variable
  size_t len = strlen(search);//get the length of the word to be searched
  int fnd = 0;//intialize a found variable to 0
  //if fnd=1---->word found & fnd=0---->word not found
  while (fnd != 1)//check if the words has been found
  {
    fnd = 1;//for temporary purpose we say that we found the word
    for (i=0; i<len; i++)//loop through the strings 'len' times
    {
      if (buffer[i] != search[i])//comare one character of the pdf text to all the characters of the searched word
      {//if not a match then
        fnd = 0;//let the fnd variable be 0
        break;//break from the loop since there is no use of searching anymore
      }
    }

    if (fnd == 1)//this checks if the word has been found
    {//if yes then,
      return buffer - buffer0;//return the position of the string in the pdf document
    }
    buffer = buffer + 1;//keep decreasing the string size from the left, like 'pizza' will become 'izza'
    if (buffer - buffer0 + len >= buffersize)//this checks if the positionof the word found is exceeding the actual document size
    {
      return -1;
    }
  }
  return -1;
}

// Keep this many previous recent characters for back reference:
#define oldchar 15

// Convert a recent set of characters into a number if there is one. Otherwise return -1:
float extract_number
(
const char* search,
int lastcharoffset
)
{
  int i = lastcharoffset;
  while (i>0 && search[i] == ' ')
  {
    i--;
  }
  while (i>0 && (isdigit(search[i]) || search[i] == '.'))
  {
    i--;
  }
  float flt = -1.0;
  char buffer[oldchar + 5];
  bzero(buffer,sizeof(buffer));
  strncpy(buffer, search+i+1, lastcharoffset-i);
  if (buffer[0] && sscanf(buffer, "%f", &flt))
  {
    return flt;
  }
  return -1.0;
}
// Check if a certain 2 character token just came along (e.g. BT):
int seen2
(
const char* search,
char* recent
)
{
  if (
    recent[oldchar-3] == search[0]
    && recent[oldchar-2] == search[1]
    && (recent[oldchar-1] == ' ' || recent[oldchar-1] == 0x0d || recent[oldchar-1] == 0x0a)
    && (recent[oldchar-4] == ' ' || recent[oldchar-4] == 0x0d || recent[oldchar-4] == 0x0a)
  )
  {
    return 0;
  }
  return -1;
}

// Function to decrypt the text that was envcrypted by adobe
void doconvert
(
char *buffer,
size_t *len_out,
char *output,
size_t len
)
{
  int intextobject = 0;//Check if we are currently inside the text object

  int nextliteral = 0;// Check if the next character is a literal (e.g. \\ to get a \ character or \( to get ( ):
  // () Bracket nesting level. Text appears inside ()
  int rbdepth = 0;

  char oc[oldchar];// Keep previous chars to get extract numbers etc.:
  size_t i;
  size_t b;
  int j = 0;

  for (j=0; j<oldchar; j++)//initialize oc to nothing at the first
  {
    oc[j] = ' ';
  }
  for (b=0, i=0; i<len; i++)//loop through the pdf text
  {
    char c = output[i];//store each character of the pdf text in char c
    if (intextobject == 1)//check if the pointer has gone inside the text object
    {//if yes then
      if (rbdepth == 0 && (seen2("TD", oc) == 0))//check if the key word is present in the text
      {
        // Positioning. See if a new line has to start or just a tab:
        float num = extract_number(oc, oldchar-5);
        if (num > 1.0)
        {
          buffer[b] = 0x0d;//adds a carriage return(\r) in the buffer
          b++;
          buffer[b] = 0x0a;//adds a new line(\n) in the buffer
          b++;
        }
        if (num<1.0)
        {
          buffer[b] = '\t';//adds a tab(\t) in the buffer
          b++;
        }
      }
      if (rbdepth == 0 && (seen2("ET", oc) == 0))
      {
        // End of a text object, also go to a new line.
        intextobject = 0;
        buffer[b] = 0x0d;//adds a carriage return(\r) in the buffer
        b++;
        buffer[b] = 0x0a;//adds a new line(\n) in the buffer
        b++;
      }
      else if (c == '(' && rbdepth == 0 && (nextliteral != 1))
      {
        // Start outputting text!

        rbdepth=1;
        // See if a space or tab (>1000) is called for by looking at the number in front of (
        int num = extract_number(oc, oldchar - 1);
        if (num > 0)
        {
          if (num > 1000.0)
          {
            buffer[b] = '\t';
            b++;
          }
          else if (num > 100.0)
          {
            buffer[b] = ' ';
            b++;
          }
        }
      }
      else if (c == ')' && rbdepth == 1 && (nextliteral != 1))
      {
        // Stop outputting text.
        rbdepth = 0;
      }
      else if (rbdepth == 1)
      {
        // Just a normal text character:
        if (c == '\\' && (nextliteral != 1))
        {
          // Only print out next character no matter what. Do not interpret.
          nextliteral = 1;
        }
        else
        {
          nextliteral = 0;
          if (((c >= ' ') && (c <= '~')) || ((c >= 128) && (c < 255)))
          {
            buffer[b] = c;
            b++;
          }
        }
      }
    }
    for (j=0; j<oldchar-1; j++)
    {
      oc[j] = oc[j+1];
    }
    oc[oldchar-1] = c;
    if (intextobject != 1)
    {
      if (seen2("BT", oc) == 0)
      {
        // Start of a text object:
        intextobject = 1;
      }
    }
  }
  *len_out = b;
}
// Extern function:
void pdftotxt
(
char *buf_out, // prepared buffer to fill with text
size_t *len_out, // pointer that is to be returned size of read text
char *buf_in, // input buffer of _prepared_ pdf data
long len_in // size of input buffer
)
{
  int morestreams = 1;
  size_t totout = 0;
  size_t outsize = 0;
  size_t prevoutsize = 0;
  char *reout;
  char *out = NULL;
  // Now search the buffer repeated for streams of data:
  while (morestreams == 1)
  {
    // Search for stream, endstream. We ought to first check the filter of the object to make sure it if FlateDecode, but skip that for now!
    int streamstart = findstringinbuffer(buf_in, "stream", len_in);//gets the starting position for the word "Stream"
    int streamend = findstringinbuffer(buf_in, "endstream", len_in);//gets the starting position for the words "Endstream"
    if (streamstart > 0 && streamend > streamstart)//checks if there are any streams
    {
      streamstart += 6;//since stream is a 6 letter words we skip 6 positions
      //start to check content inside the stream
      if (buf_in[streamstart] == 0x0d && buf_in[streamstart + 1] == 0x0a)//checks if there are '\n' and '\r' in the stream
      {
        streamstart += 2;//if yes, then skip those characters
      }
      else if (buf_in[streamstart] == 0x0a)//checks if '\n' only is present
      {
        streamstart++;//if yes, then skip this character
      }
      if (buf_in[streamend - 2] == 0x0d && buf_in[streamend-1] == 0x0a)//checks if the there are '\n'&'\r' in the second last and last positions of the stream
      {
        streamend -= 2;//if yes, then skip those characters from the end
      }
      else if (buf_in[streamend - 1] == 0x0a)//checks if '\n' only is present at the last position
      {
        streamend--;//if yes, then skip this character from the end
      }
      outsize = (streamend - streamstart) * OUTPUT_BUFFER;//after eliminating '\n'&'\r' get the size(streamend -streamstart) and multiply if by 10 for safety reasons
      char* output;
      output = (char *)malloc(outsize * sizeof(char));//initialize a string 'output' of size outsize
      bzero(output, outsize);//initializes the string to zero

      // We start using the zlib library
      z_stream zstrm;//create a variable of type z_stream
      bzero(&zstrm, sizeof(zstrm));//initialize zstrm to zero
      zstrm.avail_in = streamend - streamstart + 1;//store the stream size in avail_in
      zstrm.avail_out = outsize;//store the free space of stream remaining in avail_out
      zstrm.next_in = (Bytef*)(buf_in + streamstart);
      zstrm.next_out = (Bytef*)output;
      int rsti = inflateInit(&zstrm);
      if (rsti == Z_OK)
      {
        int rst2 = inflate(&zstrm, Z_FINISH);
        if (rst2 >= 0)
        {
          // Ok, got something, extract the text:
          prevoutsize = totout;
          totout = totout + zstrm.total_out;
          reout = (char *)realloc(out, totout * sizeof(char));
          if (reout != NULL)
          {
            out = reout;
          }
          else
          {
            free(out);
            exit(1);
          }
          memcpy(out+prevoutsize, output, zstrm.total_out);
        }
      }
      free(output);
      output = 0;
      buf_in += streamend + 7;
      len_in -= streamend + 7;
    }
    else
    {
      morestreams = 0;
    }
  }
  // Convert buffer to ascii text:
  doconvert(buf_out, len_out, out, totout);
  free(out);
}
