#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sstream>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>

namespace lib {

class Socket {
  public:
    virtual int Connect(const char *, int) = 0;
    virtual int Write(const char *, int) = 0;
    virtual int Read(char *, int) = 0;
};

typedef std::vector<char> HttpBody;

struct HttpPacket {
  char *data;
  int size;
};

class TcpSocket : public Socket {
  public:
    TcpSocket() {
      m_handle = socket(AF_INET, SOCK_STREAM, 0);
    };
    ~TcpSocket() {
      close(m_handle);
    };

    int Connect(const char *hostname, int port) {
      hostent *he = gethostbyname(hostname);
      sockaddr_in sa;

      if(he == NULL)
        return -1;

      memset(&sa, 0, sizeof(sa));
      sa.sin_family = AF_INET;
      sa.sin_port = htons(port);
      sa.sin_addr.s_addr = *(long *)he->h_addr;

      int result = connect(m_handle, (sockaddr*)&sa, sizeof(sa));

      if(result < 0)
        return -1;

      return 0;
    };

    int Write(const char *data, int length) {
      return send(m_handle, data, length, 0);
    };

    int Read(char *data, int length) {
      return recv(m_handle, data, length - 1, 0);
    };

  private:
    int m_handle;
};

class UrlString {
  public:
    UrlString(const char *url) {
      std::string surl(url);
      m_host = "127.0.0.1";
      m_path = "/system";
      m_protocol = "http";
      m_port = 80;
    };
    std::string Host() { return m_host; }
    std::string Protocol() { return m_protocol; }
    std::string Path() { return m_path; }
  private:
    std::string m_host;
    std::string m_path;
    int m_port;
    std::string m_protocol;
};

class HttpResponse {
  public:
    HttpResponse() = default;
    HttpResponse(char *data, int size) {
      char *head_break = strstr(data, "\r\n\r\n");
      int head_size = head_break - data,
          body_size = size - (head_size + 4);

      m_body.resize(body_size);
      memcpy(&m_body[0], &data[head_size + 4], body_size * sizeof(char));
    };
  private:
    std::vector< std::pair<std::string, std::string> > m_headers;
    std::vector<char> m_body;
};

class HttpRequest {
  public:
    HttpRequest(const UrlString& url) : m_url(url) {
    };

    template <class TT_Reader>
    int Send(TT_Reader& reader) {
      TcpSocket socket;

      int ok = socket.Connect(m_url.Host().c_str(), 1337);

      if(ok < 0)
        return ok;

      std::stringstream req_str;
      req_str << "GET " << m_url.Path() << " HTTP/1.1\n";
      req_str << "Host: " << m_url.Host() << "\n";
      req_str << "Content-Length: 0";
      req_str << "\r\n\r\n";

      std::string rs = req_str.str();

      int written = socket.Write(rs.c_str(), rs.size());

      if(written != rs.size())
        return -1;

      int received;

      do {
        char *buffer = (char*) malloc(2048 * sizeof(char));
        memset(buffer, '\0', 2047);

        received = socket.Read(buffer, 2047);

        buffer = (char*) realloc(buffer, (received + 1) * sizeof(char));

        buffer[received] = '\0';

        HttpPacket p;
        p.data = buffer;
        p.size = received;
        reader << p;

        free(buffer);
      } while(received > 0 && reader.Count() == 0);

      return ok;
    }

  private:
    UrlString m_url;
};

class HttpReader {
  public:

    HttpReader() {
      m_impl = new HttpReaderImpl();
    };

    HttpReader(const HttpReader& other) {
      m_impl = other.m_impl;
      m_impl->m_refcount++;
    };

    HttpReader& operator=(const HttpReader& other) {
      Deref();
      m_impl = other.m_impl;
      m_impl->m_refcount++;
      return *this;
    };

    ~HttpReader() {
      Deref();
    };

    int Count() { 
      return m_responses.size();
    };

    HttpReader& operator <<(HttpPacket p) {
      if(*m_impl << p) {
        m_responses.push_back(HttpResponse(m_impl->m_current_buffer, m_impl->m_current_size));
        m_impl->Flush();
      }
      return *this;
    };

  private:
    void Deref() {
      if(--m_impl->m_refcount <= 0) delete m_impl;
    }

    class HttpReaderImpl {
      friend class HttpReader;

      public:
        HttpReaderImpl() : m_current_size(0), m_refcount(0), m_current_content_length(-1) {
          m_current_buffer = (char*) malloc(sizeof(char));
          m_current_buffer[0] = '\0';
        };

        ~HttpReaderImpl() {
          free(m_current_buffer);
          m_current_buffer = 0;
        };

        bool operator <<(HttpPacket p) {
          m_current_buffer = (char*) realloc(m_current_buffer, sizeof(char) * (p.size + m_current_size + 1));
          memcpy(&m_current_buffer[m_current_size], &p.data[0], sizeof(char) * p.size);
          m_current_size += p.size;
          m_current_buffer[m_current_size] = '\0';
          return HasRequest();
        };

        void Flush() {
          if(m_current_buffer) free(m_current_buffer);
          m_current_buffer = 0;
          m_current_size = 0;
          m_current_content_length = -1;
        };

      private:
        int HeadSize() {
          char *newline = strstr(m_current_buffer, "\r\n\r\n");

          if(newline == nullptr)
            return -1;

          return newline - m_current_buffer;
        };

        bool HasRequest() {
          int head_size = HeadSize();

          if(head_size < 0)
            return false;

          std::string headers;
          std::string line;
          headers.append(m_current_buffer, head_size);
          std::istringstream linestream(headers);

          while(std::getline(linestream, line) && line != "\r") {
            int split = line.find(':', 0);

            if(split > line.size()  - 2 || split == 0)
              continue;

            std::string key = line.substr(0, split),
                        val = line.substr(split + 2);

            if(key.find("Content-Length") == 0 && key.size() == 14)
              m_current_content_length = std::stoi(val);
          }

          if(m_current_content_length >= 0)
            return (m_current_size - (head_size + 4)) == m_current_content_length;

          return false;
        }
        int m_refcount;
        char *m_current_buffer;
        int m_current_size;
        int m_current_content_length;
    };

    HttpReaderImpl *m_impl;
    std::vector<HttpResponse> m_responses;
};

class JsonBody {
};

};


int main(int argc, char* argv[]) {
  lib::HttpRequest req(lib::UrlString("http://meida.lofti.li/txt.txt"));
  lib::HttpReader r;
  req.Send(r);
}
