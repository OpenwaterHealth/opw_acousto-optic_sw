#include "execnode.h"

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


  char s1[80] = "1 2 3 4 5 6 7 8";
  char s2[80] = "a b c d e f g h";
  printers[0].Produce(s1);
  printers[0].Consume(s2);



  system("pause");
  return 0;
}
