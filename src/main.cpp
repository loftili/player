#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// #include <iostream>

template <class TT_Message>
class T_StdinReceiver {
  public:
    TT_Message Receive() {
      char buffer[100];
      printf("please enter a message: ");
      fgets(buffer, 99, stdin);
      printf("received[%s]\n", Trim(buffer));
      TT_Message m(buffer);
      return m;
    };
  private:
    char* Trim(char* str) {
      char* nl = strchr(str, '\n');

      if(nl)
        *nl = '\0';

      return str;
    }
};

class SimpleMessage {
  private:
    class SimpleMessageRef;

  public:
    enum COMMAND { START, STOP, EXIT };

  public:
    SimpleMessage();
    SimpleMessage(char* msg_data);
    ~SimpleMessage();
    SimpleMessage(const SimpleMessage& other);
    SimpleMessage& operator=(const SimpleMessage& other);
    void Print();
    SimpleMessage::COMMAND Translate();

  private:
    SimpleMessageRef* m_ref;
};

class SimpleMessage::SimpleMessageRef {
  public:
    SimpleMessageRef() : m_count(0) {
      m_data = new char[1];
      m_data[0] = '\0';
    };
    SimpleMessageRef(char* data) : m_count(0) {
      int length = strlen(data);
      printf("initializing m_data to [%s] which is [%d] long \n", data, length);
      m_data = new char[length + 1];
      strcpy(m_data, data);
      m_data[length] = '\0';
    };
    ~SimpleMessageRef() {
      printf("deleting message ref [%s]\n", m_data);
      delete m_data;
    };
    char* m_data;
    int m_count;
};

SimpleMessage::SimpleMessage() {
  printf("default constructor\n");
}

SimpleMessage::SimpleMessage(char* msg_data) {
  printf("creating new simple message from char pointer\n");
  m_ref = new SimpleMessage::SimpleMessageRef(msg_data);
  m_ref->m_count++;
}

SimpleMessage::SimpleMessage(const SimpleMessage& other) { 
  printf("creating new simple message from other simple message\n");
  m_ref = other.m_ref;
  m_ref->m_count++;
}
SimpleMessage& SimpleMessage::operator=(const SimpleMessage& other) { 
  printf("assignment of one simplemessage to another\n");
  m_ref = other.m_ref;
  m_ref->m_count++;
  return *this; 
}

SimpleMessage::~SimpleMessage() {
  printf("cleaning up a simple message\n");
  if(--m_ref->m_count == 0) delete m_ref;
}

void SimpleMessage::Print() {
  printf("[%s]\n", m_ref->m_data);
}

SimpleMessage::COMMAND SimpleMessage::Translate() {
  char* point = strstr("exit", m_ref->m_data);

  if(point) return EXIT;

  return START;
};

template <template <class> class TT_Receiver, class TT_Message>
class DispatchEngine : public TT_Receiver<TT_Message> {
};


int main(int argc, char* argv[]) {
  DispatchEngine<T_StdinReceiver, SimpleMessage> dispatch;

  SimpleMessage::COMMAND c = SimpleMessage::START;

  while(c != SimpleMessage::EXIT) {
    SimpleMessage s = dispatch.Receive();
    s.Print();
    c = s.Translate();
  }

  return 0;
}
