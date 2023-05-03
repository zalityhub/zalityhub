// ONLY FOR WINDOWS 
// NEEDS WINDOWS SET UP 
// COMPILE USING Visual Studio Express, 
// Visual C++ Express any edition 
// any version 
#pragma comment(lib, "xmllite.lib")

#pragma comment(lib, "urlmon.lib")

#include <urlmon.h>

#include <xmlLite.h>

#include <iostream>

#define getURL URLOpenBlockingStreamA

// only two tags to be read 
#define TITLE "title"

#define DESCR "description"

using namespace std;

int main()
{

  // Step 1 get the RSS XML Stream // 
  // Windows IStream interface 
  IStream* stream;

  // the address of the RSS feed 
  // we are using bbc, but you can 
  // use any other 
  const char* URL =
  "http://feeds.bbci.co.uk/"
  "news/world/rss.xml";

  cout << "Please wait. . . \r\n";

  // make a call to the URL 
  // a non-zero return means some error 
  if (getURL(0, URL, &stream, 0, 0))
  {

    cout << "Error occured.";

    cout << "Check internet";

    cout << "Check URL. Is it correct?";

    return -1;

  }

  // Step 2 create an XML reader // 
  IXmlReader* rdr;

  // windows API function that inits 
  // the IXMLReader interface so that 
  // we can read the XML tags in a loop 
  // takes 3 arguments. first is the 
  // interface identifier for the 
  // windows DLL present on your 
  // computer, the second is the pointer 
  // to the reader to be initialized 
  // the third argument is kept zero 
  if(CreateXmlReader(
  __uuidof(IXmlReader),
  reinterpret_cast<void**>(&rdr),
  0))
  {

    printf("Error. Try later");

    return -1;

  }

  rdr->SetInput(stream);

  // to support unicode text we use 
  // widechar instead of char 
  const wchar_t* el = 0;

  XmlNodeType nodeType;

  while (S_OK == rdr->Read(&nodeType))
  {

    switch (nodeType)
    {

    case XmlNodeType_Element:
      {

        // read the name of the element 
        // into el 
        rdr->GetLocalName(&el, 0);

      }

      break;

    case XmlNodeType_Text:
    case XmlNodeType_CDATA:
      {

        if(el)
        {

          const wchar_t* val = 0;

          // read the tag value 
          // inner text 
          rdr->GetValue(&val, 0);

          // if the element is the 
          // title or description tag, 
          // display it 
          if(!_wcsicmp(el, TITLE))
          {

            wcout << val << "\r\n\r\n";

          }else if(!wcsicmp(el, DESCR))

          {

            wcout << val << "\r\n\r\n";

            wcout << "***************";

            wcout << "***************";

            wcout << "***************";

            wcout << "*******\r\n\r\n";

          }

          el = 0;

        }

      }

    default:
      break;

    }

  }

  stream->Release();

  rdr->Release();

  return 0;

}
