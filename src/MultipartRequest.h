#ifndef Multipartrequest_h
#define Multipartrequest_h

#include <Print.h>
#include <FS.h>

const String BOUNDARY = "fc84f924-4916-43be-a0ca-51e3844cf159";
const String CONTENT_LENGTH = "Content-Length: ";
const String MULTIPART_HEADER = "Content-Type: multipart/form-data; boundary=";
const String CONTENT_DISPOSITION_HEADER = "Content-Disposition: form-data; name=\"";
const String OCTET_STREAM_HEADER = "Content-Type: application/octet-stream";
const String DDASH = "--";
const String QUOTE = "\"";
const String NL = "\r\n";
const String FILE_NAME = "\"; filename=\"";

class MultipartRequest
{
public:
  MultipartRequest(const String &boundary = BOUNDARY)
  {
    this->boundary = boundary;
  }

  void addText(const String &name, const String &t)
  {
    String data = DDASH;
    data += boundary;
    data += NL;
    data += CONTENT_DISPOSITION_HEADER + name + QUOTE + NL + NL;
    data += t;
    data += NL;

    textData[text_len] = data;
    text_len++;
    data_len += data.length();
  }

  void addFile(const String &name, File &f)
  {
    file[file_len] = &f;
    String data = DDASH + boundary + NL;
    data += CONTENT_DISPOSITION_HEADER + name + FILE_NAME + String(f.name()) + QUOTE + NL;
    data += OCTET_STREAM_HEADER + NL + NL;
    fileData[file_len] = data;
    file_len++;
    data_len += data.length() + f.size() + 2;
  }

  void printTo(Print *printer)
  {
    unsigned long c_len = calculateContentLength();
    
    printer->print(MULTIPART_HEADER);
    printer->println(boundary);
    printer->print(CONTENT_LENGTH);
    printer->println(c_len);
    printer->println();
    for (uint8_t i = 0; i < text_len; i++)
    {
      printer->print(textData[i]);
    }
    for (uint8_t i = 0; i < file_len; i++)
    {
      printer->print(fileData[i]);
      uint32_t pos = file[i]->position();
      file[i]->seek(0);
      uint8_t dataBuffer[128];
      while (file[i]->available())
      {
        while (file[i]->position() < file[i]->size())
        {
          size_t len = file[i]->readBytes((char *)dataBuffer, sizeof(dataBuffer));
          printer->write(dataBuffer, len);
        }
      }
      file[i]->seek(pos);
      printer->print(NL);
    }
    printer->print(DDASH);
    printer->print(boundary);
    printer->print(DDASH);
    printer->print(NL);
    printer->print(NL);
  }

private:
  String boundary;
  File *file[10];
  String fileData[10];
  uint8_t file_len = 0;
  String textData[50];
  uint8_t text_len = 0;
  uint32_t data_len = 0;
  uint32_t calculateContentLength()
  {
    uint32_t c_len = data_len;
    c_len += boundary.length();
    c_len += 8;
    return c_len;
  }
};

#endif //Multipartrequest_h