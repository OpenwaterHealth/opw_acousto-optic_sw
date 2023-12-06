#include "system/component/inc/syncnode.h"

#include <cstring>

class Printer;

std::vector<Printer> printers(4);

template <typename T>
int idx_p(std::vector<T>& v, const T* elt) {
  for (int i = 0; i < v.size(); ++i) {
    if (&v[i] == elt) return i;
  }
  return -1;
}


class Printer : public ExecNode {
  void* Exec(void* data) override {
    printf("I am %d, String is: %s\n",
      idx_p<Printer>(printers, this),
      (char*)data);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return (void*)((char*)data + 2);
  }

  void AtExit(void* data) override { 
    printf("Exiting %d, String is: %s\n", 
      idx_p<Printer>(printers, this),
           (char*)data);
  }
};

// for any node, string 1 must be before string 2
// all nodes must execute before cleanup
// for the synchronized node (printers[3]), both node 1 and node 2 
//   must finish first.
//
// printers[1] and printers[2] can execute in either order
int main() {
  printers[1].AddProducer(&printers[0]);
  printers[2].AddProducer(&printers[0]);
  printers[3].AddProducer(&printers[1]);
  printers[3].AddProducer(&printers[2]);
  SyncNode sn;
  sn.AddProducer(&printers[3]);

  char s1[80] = "1 2 3 4 5 6 7 8";
  char s2[80] = "a b c d e f g h";

  char* s;

  printf("Start Indefinite wait\n");
  printers[0].Consume((void*)s1);
  s = (char*)sn.Wait();
  if (s == NULL) {
    printf("Error: expected string\n");
  } else {
    printf("String: %s\n", s);
  }
  
  printf("Start 1s wait\n");
  printers[0].Consume((void*)s2);
  s = (char*)sn.Wait(1.0);
  if (s == NULL) {
    printf("Wait timout successful\n");
  } else {
    printf("Error: wait timout %s\n", s);
  }

  system("pause");
  return 0;
}
